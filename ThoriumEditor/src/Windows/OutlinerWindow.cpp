
#include "OutlinerWindow.h"
#include "EditorEngine.h"
#include "EditorWindow.h"
#include "EngineThread.h"
#include "Game/World.h"
#include "Game/Entity.h"
#include "Assets/Scene.h"
#include "UndoActions/SceneUndoActions.h"
#include <Util/Map.h>
#include <Util/KeyValue.h>

#include <QLineEdit>
#include <QTreeWidget>
#include <QBoxLayout>
#include <QDropEvent>
#include <QPushButton>
#include <QMenu>
#include <QTimer>
#include <QUndoStack>

class COutlinerTreeWidget : public QTreeWidget
{
public:
	COutlinerTreeWidget(QWidget* parent = nullptr) : QTreeWidget(parent) 
	{
		outliner = qobject_cast<COutlinerWindow*>(parent);
	}

protected:
	void dragEnterEvent(QDragEnterEvent* event);
	void dropEvent(QDropEvent* event);

private:
	QTreeWidgetItem* draggedItem = nullptr;
	COutlinerWindow* outliner = nullptr;
};

void COutlinerTreeWidget::dragEnterEvent(QDragEnterEvent* event)
{
	draggedItem = (TTreeDataItem<CEntity*>*)currentItem();
	QTreeWidget::dragEnterEvent(event);
}

void COutlinerTreeWidget::dropEvent(QDropEvent* event)
{
	QModelIndex index = indexAt(event->pos());
	if (!index.isValid())
	{
		if (draggedItem)
		{
			if (draggedItem->data(1, Qt::UserRole).toInt() == EItemTypes_Entity)
			{
				CEntity* ent = (CEntity*)draggedItem->data(0, Qt::UserRole).value<SizeType>();
				if (draggedItem->parent()->data(1, Qt::UserRole).toInt() == EItemTypes_Folder)
				{
					draggedItem->parent()->removeChild(draggedItem);
					outliner->sceneItem->addChild(draggedItem);

					outliner->entityFolderLut.erase(ent->EntityId());
				}
				else
				{
					CSceneComponent* prevParent = ent->RootComponent()->GetParent();
					ent->RootComponent()->Detach();

					gEditorWindow->sceneUndoStack->push(new CmdReparentComponent(ent->RootComponent(), prevParent));
				}
			}
			else if (draggedItem->data(1, Qt::UserRole).toInt() == EItemTypes_Folder)
			{
				if (draggedItem->parent())
				{
					draggedItem->parent()->removeChild(draggedItem);
					//addTopLevelItem(draggedItem);
					outliner->sceneItem->addChild(draggedItem);
				}
			}
		}
		return;
	}

	if (draggedItem)
	{
		QTreeWidgetItem* targetItem = itemFromIndex(index);
		if (targetItem)
		{
			if (draggedItem->data(1, Qt::UserRole).toInt() == EItemTypes_Entity && targetItem->data(1, Qt::UserRole).toInt() == EItemTypes_Entity)
			{
				CEntity* ent = (CEntity*)draggedItem->data(0, Qt::UserRole).value<SizeType>();
				CSceneComponent* prevParent = ent->RootComponent()->GetParent();
				CEntity* target = (CEntity*)targetItem->data(0, Qt::UserRole).value<SizeType>();
				if (target->GetWorld() == ent->GetWorld())
				{
					ent->RootComponent()->AttachTo(target->RootComponent());
					gEditorWindow->sceneUndoStack->push(new CmdReparentComponent(ent->RootComponent(), prevParent));
				}
			}
			else if (targetItem->data(1, Qt::UserRole).toInt() == EItemTypes_Folder)
			{
				if (draggedItem->parent())
					draggedItem->parent()->removeChild(draggedItem);
				else
					invisibleRootItem()->removeChild(draggedItem);

				targetItem->addChild(draggedItem);
				CEntity* ent = (CEntity*)draggedItem->data(0, Qt::UserRole).value<SizeType>();
				outliner->entityFolderLut[ent->EntityId()] = targetItem;
			}
		}
	}
}

COutlinerWindow::COutlinerWindow(QWidget* parent /*= nullptr*/) : ads::CDockWidget("Scene Outliner", parent)
{
	QWidget* pWidget = new QFrame(this);
	QVBoxLayout* layout = new QVBoxLayout(pWidget);
	layout->setContentsMargins(6, 6, 6, 6);

	setWidget(pWidget);

	setObjectName("outliner_widget");

	setIcon(QIcon(":/icons/wnd_outliner.svg"));

	filter = new QLineEdit(this);
	filter->setPlaceholderText("Search...");
	filter->setMinimumHeight(24);
	filter->setStyleSheet("QLineEdit { border-radius: 10px; }");

	outlinerTree = new COutlinerTreeWidget(this);
	outlinerTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
	outlinerTree->setSelectionBehavior(QAbstractItemView::SelectRows);
	outlinerTree->setDragDropMode(QAbstractItemView::InternalMove);
	outlinerTree->setDragEnabled(true);
	outlinerTree->setContextMenuPolicy(Qt::CustomContextMenu);
	//outlinerTree->setAcceptDrops(true);

	outlinerTree->setEditTriggers(QAbstractItemView::NoEditTriggers);

	outlinerTree->setColumnCount(2);
	outlinerTree->setHeaderLabels({ "Name", "Type" });

	sceneItem = new QTreeWidgetItem(outlinerTree);
	sceneItem->setText(1, "Scene");
	sceneItem->setData(1, Qt::UserRole, QVariant(EItemTypes_World));
	sceneItem->setExpanded(true);

	layout->addWidget(filter);
	layout->addWidget(outlinerTree);

	connect(outlinerTree, &QTreeWidget::itemSelectionChanged, this, [=]() {
		auto items = outlinerTree->selectedItems();
		
		TArray<CObject*> selectedObjects;

		for (auto* i : items)
		{
			QTreeWidgetItem* item = i;

			if (item->data(1, Qt::UserRole).toInt() == EItemTypes_Entity)
			{
				auto ent = item->data(0, Qt::UserRole).toULongLong();
				selectedObjects.Add((CEntity*)ent);
				continue;
			}
			if (item->data(1, Qt::UserRole).toInt() == EItemTypes_World)
			{
				selectedObjects.Add(gWorld);
				continue;
			}
		}

		gEditorEngine->SelectObjects(selectedObjects);
	});
	connect(outlinerTree, &QTreeWidget::customContextMenuRequested, this, [=](const QPoint& point) {
		QTreeWidgetItem* item = outlinerTree->itemAt(point);
		if (item)
		{
			if (item->data(1, Qt::UserRole).toInt() == EItemTypes_World)
			{
				QMenu menu(this);
				menu.addAction(QIcon(":/icons/entity.svg"), "Add Entity...");
				menu.addAction("Add Sub Scene...");
				menu.exec(QCursor::pos());
			};
			if (item->data(1, Qt::UserRole).toInt() == EItemTypes_Entity)
			{
				CEntity* ent = (CEntity*)item->data(0, Qt::UserRole).value<SizeType>();
				//gEditorWindow->DoEntityContextMenu(ent, outlinerTree->mapToGlobal(point));
				gEditorWindow->DoEntityContextMenu(ent, QCursor::pos());
			}
			else if (item->data(1, Qt::UserRole).toInt() == EItemTypes_Folder)
			{
				QMenu menu(this);
				menu.addAction("Rename", this, [=]() { outlinerTree->editItem(item, 0); });
				menu.addAction("Delete", this, [=]() {
					auto children = item->takeChildren();
					if (item->parent())
					{
						item->parent()->addChildren(children);
						item->parent()->removeChild(item);
					}
					else
					{
						outlinerTree->invisibleRootItem()->addChildren(children);
						outlinerTree->invisibleRootItem()->removeChild(item);
					}
				});

				menu.exec(QCursor::pos());
			}
		}
		else
		{
			QMenu menu(this);

			menu.addAction(QIcon(":/icons/folder-small.svg"), "New Folder", this, [=]() {
				QTreeWidgetItem* folder = new QTreeWidgetItem();
				folder->setIcon(0, QIcon(":/icons/folder-small.svg"));
				folder->setText(0, "New Folder");
				folder->setText(1, "Folder");
				folder->setData(1, Qt::UserRole, QVariant(EItemTypes_Folder));
				folder->setFlags(folder->flags() | Qt::ItemIsEditable);
				
				//outlinerTree->addTopLevelItem(folder);
				sceneItem->addChild(folder);
				outlinerTree->editItem(folder, 0);
			});
			menu.addAction(QIcon(":/icons/entity.svg"), "New Entity...", this, [=]() { gEditorWindow->CreateEntityPopup(); });

			menu.exec(QCursor::pos());
		}
	});
	connect(outlinerTree, &QTreeWidget::itemDoubleClicked, this, [=](QTreeWidgetItem* item, int column) {
		if (column == 0)
			outlinerTree->editItem(item, 0);
	});
	connect(outlinerTree, &QTreeWidget::itemChanged, this, [=](QTreeWidgetItem* item, int column) {
		if (column == 0 && item->data(1, Qt::UserRole).toInt() == EItemTypes_Entity)
		{
			CEntity* ent = (CEntity*)item->data(0, Qt::UserRole).value<SizeType>();
			FString prevName = ent->Name();
			ent->SetName(item->text(0).toStdString());

			gEditorWindow->sceneUndoStack->push(new CmdRenameEntity(ent, prevName));
		}
	});

	updateTimer = new QTimer(this);
	connect(updateTimer, &QTimer::timeout, this, &COutlinerWindow::Update);

	updateTimer->start(50);

	//connect(gEngineThread, &CEngineThread::onUpdate, this, &COutlinerWindow::Update);
	connect(gEngineThread, &CEngineThread::onSelectionChanged, this, &COutlinerWindow::selectionChanged);
	connect(gEditorWindow, &CEditorWindow::onSaveScene, this, &COutlinerWindow::SaveSceneTree);
	connect(gEngineThread, &CEngineThread::onLevelChanged, this, &COutlinerWindow::LoadSceneTree);
}

COutlinerWindow::~COutlinerWindow()
{
	updateTimer->stop();
	updateTimer->deleteLater();
	filter->deleteLater();
	outlinerTree->deleteLater();
}

void COutlinerWindow::Update()
{
	if (!gWorld)
		return;

	if (gWorld != curWorld)
		Clear();
	curWorld = gWorld;

	auto ents = gWorld->GetEntities();

	outlinerTree->blockSignals(true);
	FString worldName = "Empty Scene";
	if (gWorld->GetScene() && gWorld->GetScene()->File())
		worldName = gWorld->GetScene()->File()->Name();
	if (!gWorld->GetScene() || gWorld->GetScene()->IsDirty())
		worldName += '*';

	if (sceneItem->text(0) != worldName.c_str())
		sceneItem->setText(0, worldName.c_str());

	auto items = entityItems;
	for (auto it = items.begin(); it != items.end(); it++)
	{
		if (it->first && ents.find(it->first) == ents.end())
		{
			if (it->second->parent())
				it->second->parent()->removeChild(it->second);
			else
				sceneItem->removeChild(it->second);
			entityItems.erase(it->first);

			delete it->second;
		}
	}
	outlinerTree->blockSignals(false);

	for (auto& it : ents)
	{
		CEntity* ent = it.second;
		if (ent->bEditorEntity)
			continue;

		QTreeWidgetItem* entItem = entityItems[ent->EntityId()];
		if (!entItem)
		{
			auto owner = ent->GetOwner<CEntity>();
			if (owner)
				entItem = new QTreeWidgetItem(entityItems[owner->EntityId()]);
			else
				entItem = new QTreeWidgetItem();

			entItem->setFlags(entItem->flags() | Qt::ItemIsEditable);
			entItem->setData(0, Qt::UserRole, QVariant((SizeType)ent));
			entItem->setData(1, Qt::UserRole, QVariant(EItemTypes_Entity));
			entItem->setText(0, ent->Name().c_str());
			entItem->setText(1, ent->GetClass()->GetName().c_str());

			if (auto it = entityFolderLut.find(ent->EntityId()); it != entityFolderLut.end())
				it->second->addChild(entItem);

			entityItems[ent->EntityId()] = entItem;
			if (!ent->RootComponent() || ent->RootComponent()->GetParent() == nullptr)
				sceneItem->addChild(entItem);
		}
		else
		{
			auto* parentItem = entItem->parent();

			outlinerTree->blockSignals(true);
			if (entItem->text(0) != ent->Name().c_str())
				entItem->setText(0, ent->Name().c_str());
			outlinerTree->blockSignals(false);

			if (!ent->RootComponent()->GetParent() && parentItem && parentItem->data(1, Qt::UserRole).toInt() == EItemTypes_Entity)
			{
				parentItem->removeChild(entItem);
				sceneItem->addChild(entItem);
			}
			else if (CSceneComponent* parent = ent->RootComponent()->GetParent())
			{
				CEntity* owner = parent->GetEntity();

				auto newParent = entityItems[owner->EntityId()];
				if (newParent && newParent != parentItem)
				{
					if (parentItem)
						parentItem->removeChild(entItem);
					else
						sceneItem->removeChild(entItem);
					newParent->addChild(entItem);
				}
			}
		}
	}
}

void COutlinerWindow::selectionChanged()
{
	outlinerTree->blockSignals(true);
	for (auto it : entityItems)
	{
		if (it.first)
			it.second->setSelected(gEditorEngine->IsObjectSelected((CEntity*)it.second->data(0, Qt::UserRole).value<SizeType>()));
	}
	outlinerTree->blockSignals(false);
}

void ReadFolderTree(KVCategory* in, FOutlinerFolder* out)
{
	for (auto* c : in->GetCategories())
	{
		out->children.Add();
		FOutlinerFolder& f = *out->children.last();
		f.name = c->GetName();
		ReadFolderTree(c, &f);
	}

	auto* ents = in->GetArray("entities");
	if (!ents)
		return;

	for (auto& ent : *ents)
		out->entities.Add(std::stoull(ent.c_str()));
}

void AddFolderToTree(FOutlinerFolder* folder, QTreeWidgetItem* parent, TMap<SizeType, QTreeWidgetItem*>& outLut)
{
	auto* item = new QTreeWidgetItem(parent);
	item->setIcon(0, QIcon(":/icons/folder-small.svg"));
	item->setText(0, folder->name.c_str());
	item->setText(1, "Folder");
	item->setData(1, Qt::UserRole, QVariant(EItemTypes_Folder));
	item->setFlags(item->flags() | Qt::ItemIsEditable);

	for (auto& ent : folder->entities)
		outLut[ent] = item;

	for (auto& f : folder->children)
		AddFolderToTree(&f, item, outLut);
}

void COutlinerWindow::LoadSceneTree()
{
	Clear();

	if (!gWorld->GetScene() || !gWorld->GetScene()->File())
		return;

	FKeyValue kv(gWorld->GetScene()->File()->GetSdkPath(".meta"));
	if (!kv.IsOpen())
		return;

	auto* folders = kv.GetCategory("folders");
	if (!folders)
		return;

	FOutlinerFolder root;
	ReadFolderTree(folders, &root);
	for (auto& c : root.children)
		AddFolderToTree(&c, sceneItem, entityFolderLut);
}

void BuildFolderTree(QTreeWidgetItem* item, FOutlinerFolder* out)
{
	for (int i = 0; i < item->childCount(); i++)
	{
		auto* child = item->child(i);
		if (child->data(1, Qt::UserRole).toInt() == EItemTypes_Folder)
		{
			FOutlinerFolder folder;
			BuildFolderTree(child, &folder);
			folder.item = child;

			out->children.Add(folder);
		}

		if (child->data(1, Qt::UserRole).toInt() == EItemTypes_Entity)
		{
			CEntity* ent = (CEntity*)child->data(0, Qt::UserRole).value<SizeType>();
			out->entities.Add(ent->EntityId());
		}
	}
}

void WriteFolderTree(FOutlinerFolder* folder, KVCategory* out)
{
	for (auto& f : folder->children)
	{
		FString name = f.item->text(0).toStdString().c_str();
		auto* cat = out->GetCategory(name, true);
		WriteFolderTree(&f, cat);
	}

	if (folder->entities.Size() == 0)
		return;

	auto* arr = out->GetArray("entities", true);

	for (auto& ent : folder->entities)
		arr->Add(FString::ToString(ent));
}

void COutlinerWindow::SaveSceneTree()
{
	while (gWorld->GetScene() == nullptr || gWorld->GetScene()->File() == nullptr)
	{
		// wait in case the scene hasn't actually been saved yet.
		QThread::currentThread()->msleep(1);
	}

	// Build folder tree
	FOutlinerFolder root;
	BuildFolderTree(sceneItem, &root);
	root.entities.Clear();

	FKeyValue kv(gWorld->GetScene()->File()->GetSdkPath(".meta"));

	auto* folders = kv.GetCategory("folders", true);
	WriteFolderTree(&root, folders);

	kv.Save();
}

void COutlinerWindow::Clear()
{
	outlinerTree->blockSignals(true);
	for (auto it = entityItems.rbegin(); it != entityItems.rend(); it++)
	{
		if (!it->first)
			return;

		if (it->second->parent())
			it->second->parent()->removeChild(it->second);
		else
			outlinerTree->invisibleRootItem()->removeChild(it->second);
	}
	entityItems.clear();
	entityFolderLut.clear();

	auto children = sceneItem->takeChildren();
	for (auto* c : children)
		delete c;

	outlinerTree->blockSignals(false);
}
