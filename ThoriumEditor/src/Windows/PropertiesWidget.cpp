
#include "PropertiesWidget.h"
#include "Widgets/PropertyEditor.h"
#include "Widgets/ClassSelectorDialog.h"
#include "Editor.h"
#include "Game/Entity.h"
#include "Game/EntityComponent.h"
#include "EngineThread.h"
#include "EditorEngine.h"

#include <QBoxLayout>
#include <QSplitter>
#include <QMenu>
#include <QPushButton>
#include <QLineEdit>

CPropertiesWidget::CPropertiesWidget(QWidget* parent /*= nullptr*/) : ads::CDockWidget("Properties", parent)
{
	setObjectName("properties_widget");

	QWidget* pWidget = new QWidget(this);
	QVBoxLayout* layout = new QVBoxLayout(pWidget);
	layout->setContentsMargins(0,0,0,0);

	setWidget(pWidget);

	split = new QSplitter(Qt::Vertical, this);
	layout->addWidget(split);

	editor = new CPropertyEditorWidget(this);
	topWidget = new QFrame(this);
	split->addWidget(topWidget);
	split->addWidget(editor);
	split->setSizes({ 0, 360 });

	QVBoxLayout* topLayout = new QVBoxLayout(topWidget);
	topLayout->setContentsMargins(0, 0, 0, 0);

	QHBoxLayout* l1 = new QHBoxLayout();

	nameEdit = new QLineEdit(this);
	nameEdit->setPlaceholderText("Name...");
	
	l1->addWidget(nameEdit);

	QPushButton* addCompBtn = new QPushButton(QIcon(":/icons/btn-add.svg"), "", this);
	addCompBtn->setProperty("type", QVariant("clear"));
	l1->addWidget(addCompBtn);

	childTree = new QTreeWidget(this);
	childTree->setHeaderHidden(true);
	childTree->setContextMenuPolicy(Qt::CustomContextMenu);

	topLayout->addLayout(l1);
	topLayout->addWidget(childTree);

	UpdateUI();

	connect(childTree, &QTreeWidget::customContextMenuRequested, this, [=](const QPoint& point) {
		QTreeWidgetItem* item = childTree->itemAt(point);
		if (item)
		{
			CObject* obj = (CObject*)item->data(0, 1000).toULongLong();
			FClass* c = obj->GetClass();

			/*if (!c->CanCast(CEntityComponent::StaticClass()))
				return;*/


			QMenu menu(this);

			menu.addAction("Add Component", this, &CPropertiesWidget::AddComponent);
			if (c->CanCast(CEntityComponent::StaticClass()))
			{
				QAction* del = menu.addAction("Delete", this, [=]() {
					CEntityComponent* comp = (CEntityComponent*)obj;
					Cast<CEntity>(targetObject)->RemoveComponent(comp);
					selectedChild = targetObject;
					editor->SetObject(targetObject);
					UpdateUI();
				});
				del->setEnabled(Cast<CEntityComponent>(obj)->IsUserCreated());
			}
			menu.exec(childTree->mapToGlobal(point));
		}
	});
	connect(childTree, &QTreeWidget::itemClicked, this, [=](QTreeWidgetItem* item, int) {
		CObject* obj = (CObject*)item->data(0, 1000).toULongLong();
		selectedChild = obj;
		editor->SetObject(obj);
		emit(ComponentSelected(selectedChild));
	});
	connect(addCompBtn, &QPushButton::clicked, this, &CPropertiesWidget::AddComponent);
	connect(nameEdit, &QLineEdit::textEdited, this, [=](const QString& n) { if (targetObject) targetObject->SetName(n.toStdString()); });
	connect(gEngineThread, &CEngineThread::onSelectionChanged, this, [=]() {
		SetObject(gEditorEngine->activeObject);
	});

}

CPropertiesWidget::~CPropertiesWidget()
{

}

void CPropertiesWidget::SetObject(CObject* obj)
{
	if (targetObject == obj || selectedChild == obj)
		return;

	targetObject = obj;
	selectedChild = obj;
	editor->SetObject(obj);
	UpdateUI();
}

void CPropertiesWidget::Update()
{
	editor->Update();

	for (auto itm : items)
	{
		CObject* obj = (CObject*)itm->data(0, 1000).toULongLong();

		QString objName = obj->Name().c_str();
		if (itm->text(0) != objName)
			itm->setText(0, objName);
	}
}

void CPropertiesWidget::UpdateUI()
{
	childTree->clear();
	sceneComps.clear();
	items.Clear();

	if (!targetObject || !targetObject->GetClass()->CanCast(CEntity::StaticClass()))
	{
		topWidget->hide();
		return;
	}

	nameEdit->setText(targetObject->Name().c_str());

	TObjectPtr<CEntity> ent = Cast<CEntity>(targetObject);

	childTree->setVisible(true);

	topWidget->show();

	QTreeWidgetItem* entItem = new QTreeWidgetItem(childTree, EItemTypes_Entity);
	entItem->setData(0, 1000, QVariant((uint64)(CObject*)targetObject));
	entItem->setText(0, ent->Name().c_str());
	items.Add(entItem);

	for (auto& it : ent->GetAllComponents())
	{
		CEntityComponent* comp = it.second;
		if (comp == ent->RootComponent())
			continue;

		if (comp->GetClass()->CanCast(CSceneComponent::StaticClass()))
		{
			QTreeWidgetItem* item = new QTreeWidgetItem(EItemTypes_SceneComponent);
			item->setData(0, 1000, QVariant((uint64)comp));
			item->setText(0, comp->Name().c_str());

			items.Add(item);

			CSceneComponent* c = (CSceneComponent*)comp;
			if (c->GetParent() == ent->RootComponent())
				entItem->addChild(item);

			sceneComps[c] = item;
		}
		else
		{
			QTreeWidgetItem* item = new QTreeWidgetItem(childTree, EItemTypes_EntityComponent);
			item->setData(0, 1000, QVariant((uint64)comp));
			item->setText(0, comp->Name().c_str());
			items.Add(item);
		}
	}

	for (auto comp : sceneComps)
	{
		// Ignore it if it has a parent.
		if (comp.second->parent())
			continue;

		CSceneComponent* parent = comp.first->GetParent();

		auto it = sceneComps.find(parent);
		if (it == sceneComps.end())
			childTree->addTopLevelItem(comp.second);
		else
			it->second->addChild(comp.second);
	}

	childTree->expandAll();
}

void CPropertiesWidget::AddComponent()
{
	CEntity* ent = CastChecked<CEntity>(targetObject);
	if (!ent)
		return;

	CClassSelectorDialog dialog(this);
	dialog.SetFilterClass(CEntityComponent::StaticClass());
	if (dialog.exec())
	{
		FClass* compClass = dialog.GetSelectedClass();
		CEntityComponent* newComp = ent->AddComponent(compClass, compClass->GetName());
		if (!newComp)
			return;

		//newComp->bUserCreated = true;

		CSceneComponent* sceneComp = Cast<CSceneComponent>(newComp);
		if (sceneComp)
		{
			if (CSceneComponent* sel = Cast<CSceneComponent>(selectedChild); selectedChild != targetObject && sel)
				sceneComp->AttachTo(sel);
			else
				sceneComp->AttachTo(ent->RootComponent(), FTransformSpace::KEEP_LOCAL_TRANSFORM);
		}
		UpdateUI();
	}
}
