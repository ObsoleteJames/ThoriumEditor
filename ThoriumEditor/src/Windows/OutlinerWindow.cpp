
#include "OutlinerWindow.h"
#include "EditorEngine.h"
#include "EditorWindow.h"
#include "EngineThread.h"
#include "Game/World.h"
#include "Game/Entity.h"
#include "UndoActions/SceneUndoActions.h"
#include <Util/Map.h>

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
	COutlinerTreeWidget(QWidget* parent = nullptr) : QTreeWidget(parent) {}

protected:
	void dragEnterEvent(QDragEnterEvent* event);
	void dropEvent(QDropEvent* event);

private:
	QTreeWidgetItem* draggedItem = nullptr;

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
				CSceneComponent* prevParent = ent->RootComponent()->GetParent();
				ent->RootComponent()->Detach();

				gEditorWindow->sceneUndoStack->push(new CmdReparentComponent(ent->RootComponent(), prevParent));

				if (draggedItem->parent()->data(1, Qt::UserRole).toInt() == EItemTypes_Folder)
				{
					draggedItem->parent()->removeChild(draggedItem);
					addTopLevelItem(draggedItem);
				}
			}
			else if (draggedItem->data(1, Qt::UserRole).toInt() == EItemTypes_Folder)
			{
				if (draggedItem->parent())
				{
					draggedItem->parent()->removeChild(draggedItem);
					addTopLevelItem(draggedItem);
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
				ent->RootComponent()->AttachTo(((CEntity*)targetItem->data(0, Qt::UserRole).value<SizeType>())->RootComponent());

				gEditorWindow->sceneUndoStack->push(new CmdReparentComponent(ent->RootComponent(), prevParent));
			}
			else if (targetItem->data(1, Qt::UserRole).toInt() == EItemTypes_Folder)
			{
				if (draggedItem->parent())
					draggedItem->parent()->removeChild(draggedItem);
				else
					invisibleRootItem()->removeChild(draggedItem);

				targetItem->addChild(draggedItem);
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

	layout->addWidget(filter);
	layout->addWidget(outlinerTree);

	connect(outlinerTree, &QTreeWidget::itemSelectionChanged, this, [=]() {
		auto items = outlinerTree->selectedItems();
		
		TArray<CObject*> selectedObjects;

		for (auto* i : items)
		{
			QTreeWidgetItem* item = i;
			auto ent = item->data(0, Qt::UserRole).toULongLong();
			
			selectedObjects.Add((CEntity*)ent);
		}

		gEditorEngine()->SelectObjects(selectedObjects);
	});
	connect(outlinerTree, &QTreeWidget::customContextMenuRequested, this, [=](const QPoint& point) {
		QTreeWidgetItem* item = outlinerTree->itemAt(point);
		if (item)
		{
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
				
				outlinerTree->addTopLevelItem(folder);
				outlinerTree->editItem(folder, 0);
			});
			menu.addAction(QIcon(":/icons/entity.svg"), "New Entity...");

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

	QTimer* updateTimer = new QTimer(this);
	connect(updateTimer, &QTimer::timeout, this, &COutlinerWindow::Update);

	updateTimer->start(50);

	//connect(gEngineThread, &CEngineThread::onUpdate, this, &COutlinerWindow::Update);
	connect(gEngineThread, &CEngineThread::onSelectionChanged, this, &COutlinerWindow::selectionChanged);
}

COutlinerWindow::~COutlinerWindow()
{
	filter->deleteLater();
	outlinerTree->deleteLater();
}

void COutlinerWindow::Update()
{
	if (!gWorld)
		return;

	auto ents = gWorld->GetEntities();

	outlinerTree->blockSignals(true);
	auto items = entityItems;
	for (auto it = items.begin(); it != items.end(); it++)
	{
		if (it->first && ents.find(it->first) == ents.end())
		{
			if (it->second->parent())
				it->second->parent()->removeChild(it->second);
			else
				outlinerTree->invisibleRootItem()->removeChild(it->second);
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
				//entItem = new TTreeDataItem<CEntity*>(ent, entityItems[owner->EntityId()]);
			else
				entItem = new QTreeWidgetItem();
				//entItem = new TTreeDataItem<CEntity*>(ent, 0);

			entItem->setFlags(entItem->flags() | Qt::ItemIsEditable);
			entItem->setData(0, Qt::UserRole, QVariant((SizeType)ent));
			entItem->setData(1, Qt::UserRole, QVariant(EItemTypes_Entity));
			entItem->setText(0, ent->Name().c_str());
			entItem->setText(1, ent->GetClass()->GetName().c_str());

			entityItems[ent->EntityId()] = entItem;
			if (!ent->RootComponent() || ent->RootComponent()->GetParent() == nullptr)
				outlinerTree->addTopLevelItem(entItem);
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
				outlinerTree->addTopLevelItem(entItem);
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
						outlinerTree->invisibleRootItem()->removeChild(entItem);
					newParent->addChild(entItem);
				}
			}
		}
	}
}

void COutlinerWindow::selectionChanged()
{
	//auto& selected = gEditorEngine()->selectedObjects;

	outlinerTree->blockSignals(true);
	for (auto it : entityItems)
	{
		if (it.first)
			it.second->setSelected(gEditorEngine()->IsObjectSelected((CEntity*)it.second->data(0, Qt::UserRole).value<SizeType>()));
	}
	outlinerTree->blockSignals(false);
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
	outlinerTree->blockSignals(false);
}
