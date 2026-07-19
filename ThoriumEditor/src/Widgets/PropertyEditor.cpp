
#include "PropertyEditor.h"
#include "Widgets/CollapsableWidget.h"
#include "PropertyEditors/ArrayProperty.h"
#include "PropertyEditors/IntProperty.h"
#include "PropertyEditors/FloatProperty.h"
#include "PropertyEditors/BoolProperty.h"
#include "PropertyEditors/EnumProperty.h"
#include "PropertyEditors/StringProperty.h"
#include "PropertyEditors/StructProperty.h"
#include "PropertyEditors/ObjectPtrProperty.h"
#include "PropertyEditors/ClassPtrProperty.h"
#include "PropertyEditors/VectorProperty.h"
#include "PropertyEditors/QuatProperty.h"
#include "EditorWindow.h"

#include "Object/PropertyHandler.h"
#include "Game/Components/SceneComponent.h"
#include "Game/Entity.h"

#include "EditorEngine.h"

#include <QLabel>
#include <QScrollArea>
#include <QBoxLayout>
#include <QUndoStack>

QUndoCommand* IBasePropertyEditor::ProvideUndoCmd()
{
	auto* cmd = curUndoCmd;
	curUndoCmd = nullptr;
	return cmd;
}

void IBasePropertyEditor::SetDefaultObject(CObject* obj)
{
	cdo = obj;
}

QPushButton* IBasePropertyEditor::AddRevertBtn(IPropertyHandler* handler)
{
	QPushButton* btn = new QPushButton(QIcon(":/icons/field_revert.svg"), "", this);
	btn->setToolTip("Revert to default value");
	btn->setProperty("type", QVariant("clear"));

	QSizePolicy sp = btn->sizePolicy();
	sp.setRetainSizeWhenHidden(true);
	btn->setSizePolicy(sp);
	btn->setVisible(false);
	btn->setMaximumSize(20, 20);
	return btn;
}

CPropertyEditorWidget::CPropertyEditorWidget(QWidget* parent /*= nullptr*/) : QWidget(parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	scrollArea = new QScrollArea(this);
	scrollArea->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QWidget* content = new QWidget(this);
	scrollLayout = new QVBoxLayout(content);
	content->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
	scrollArea->setWidget(content);
	//scrollLayout->setContentsMargins(0, 0, 0, 0);
	//scrollLayout->setSpacing(0);

	layout->addWidget(scrollArea);

	connect(gEditorWindow->sceneUndoStack, &QUndoStack::indexChanged, this, &CPropertyEditorWidget::Update);
}

void CPropertyEditorWidget::SetObject(CObject* obj)
{
	if (targetObject == obj)
		return;

	targetObject = obj; 
	RebuildUI();
}

IBasePropertyEditor* CPropertyEditorWidget::CreatePropertyEditor(void* ptr, const FProperty* p, QWidget* parent)
{
	switch (p->type)
	{
	case EVT_STRUCT:
	{
		if (p->typeName == "FVector")
			return new CVectorProperty((FVector*)((SizeType)ptr + p->offset), p->name, parent);

		return new CStructProperty(ptr, p, parent);
	}
	case EVT_CLASS:
		break;
	case EVT_STRING:
		return new CStringProperty(ptr, p, parent);
	case EVT_ENUM:
		return new CEnumProperty(ptr, p, parent);
	case EVT_ARRAY:
		return new CArrayProperty(ptr, p, parent);
	case EVT_OBJECT_PTR:
		return new CObjectPtrProperty(ptr, p, parent);
	case EVT_CLASS_PTR:
		return new CClassPtrProperty(ptr, p, parent);
	case EVT_FLOAT:
		return new CFloatProperty(ptr, p, parent);
	case EVT_DOUBLE:
		return new CFloatProperty(ptr, p, parent);
	case EVT_INT:
		return new CIntProperty((int*)ptr, p, parent);
	case EVT_UINT:
		return new CUIntProperty((uint*)ptr, p, parent);
	case EVT_BOOL:
		return new CBoolProperty((bool*)ptr, p, parent);
	}

	return nullptr;
}

void CPropertyEditorWidget::RebuildUI()
{
	for (auto* w : curWidgets)
	{
		scrollLayout->removeWidget(w);
		delete w;
	}
	curWidgets.Clear();
	properties.Clear();
	
	if (!targetObject)
		return;

	TObjectPtr<CSceneComponent> sceneComp = CastChecked<CSceneComponent>(targetObject);
	if (!sceneComp)
	{
		if (auto ent = CastChecked<CEntity>(targetObject); ent)
			sceneComp = ent->RootComponent();
	}

	if (sceneComp)
	{
		CCollapsableWidget* cat = GetCategoryWidget("Transform");

		FVector* pos = (FVector*)&sceneComp->GetPosition();
		FQuaternion* rot = (FQuaternion*)&sceneComp->GetRotation();
		FVector* scale = (FVector*)&sceneComp->GetScale();

		CVectorProperty* posProp = new CVectorProperty(pos, "Position", this);
		CQuatProperty* rotProp = new CQuatProperty(rot, "Rotation", this);
		CVectorProperty* scaleProp = new CVectorProperty(scale, "Scale", this);
		
		CSceneComponent* sceneCompPtr = sceneComp.Get();
		connect(posProp, &CVectorProperty::OnValueChanged, this, [=]() { sceneCompPtr->UpdateWorldTransform(); });
		connect(rotProp, &CVectorProperty::OnValueChanged, this, [=]() { sceneCompPtr->UpdateWorldTransform(); });
		connect(scaleProp, &CVectorProperty::OnValueChanged, this, [=]() { sceneCompPtr->UpdateWorldTransform(); });

		AddProperty(posProp, sceneComp.Get(), sceneComp->GetClass()->GetProperty("position"));
		AddProperty(rotProp, sceneComp.Get(), sceneComp->GetClass()->GetProperty("rotation"));
		AddProperty(scaleProp, sceneComp.Get(), sceneComp->GetClass()->GetProperty("scale"));

		cat->Widget()->layout()->addWidget(scaleProp);
		cat->Widget()->layout()->addWidget(rotProp);
		cat->Widget()->layout()->addWidget(posProp);
	}

	if (auto ent = CastChecked<CEntity>(targetObject); ent)
	{
		for (FClass* c = ent->GetClass(); c != nullptr; c = c->GetBaseClass())
		{
			for (const FProperty* p = c->GetPropertyList(); p != nullptr; p = p->next)
			{
				if (p->meta && p->meta->HasFlag("ExposeProperties") && p->type == EVT_OBJECT_PTR)
				{
					TObjectPtr<CObject>& target = *(TObjectPtr<CObject>*)((SizeType)&*ent + p->offset);
					FClass* type = target->GetClass();
					CObject* cdo = type->GetDefaultObject();
					if (target)
						AddProperties(target->GetClass(), target, cdo, false, p->name);
				}
			}
		}
	}
	
	FClass* type = targetObject->GetClass();
	CObject* cdo = type->GetDefaultObject();
	AddProperties(type, targetObject, cdo);
}

void CPropertyEditorWidget::Update()
{
	for (auto* p : properties)
		p->Update();
}

CCollapsableWidget* CPropertyEditorWidget::GetCategoryWidget(const FString& category)
{
	for (CCollapsableWidget* c : curWidgets)
		if (c->Text() == category.c_str())
			return c;

	CCollapsableWidget* r = new CCollapsableWidget(category.c_str(), nullptr, this);
	QWidget* content = new QWidget(r);
	QVBoxLayout* layout = new QVBoxLayout(content);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->setDirection(QBoxLayout::BottomToTop);
	r->SetWidget(content);
	r->SetCollapsed(false);

	curWidgets.Add(r);
	scrollLayout->addWidget(r);
	return r;
}

void CPropertyEditorWidget::AddProperties(FStruct* type, void* obj, CObject* cdo, bool bRecursive, const FString& overrideCat)
{
	for (const FProperty* p = type->GetPropertyList(); p != nullptr; p = p->next)
	{
		if ((p->flags & VTAG_EDITOR_EDITABLE) == 0 && (p->flags & VTAG_EDITOR_VISIBLE) == 0)
			continue;

		// This is no longer needed as IPropertyHandler takes care of this now.
		//void* ptr = (void*)((SizeType)obj + p->offset);

		if (p->meta && p->meta->HasFlag("Inline") && p->type == EVT_STRUCT)
		{
			AddProperties(CModuleManager::FindStruct(p->typeName), (void*)((SizeType)obj + p->offset));
			continue;
		}

		bool readOnly = (p->flags & VTAG_EDITOR_EDITABLE) == 0;

		FString catName = type->GetName();
		if (p->meta && !p->meta->category.IsEmpty())
			catName = p->meta->category;

		if (!overrideCat.IsEmpty())
			catName = overrideCat;
		
		CCollapsableWidget* cat = GetCategoryWidget(catName);
		IBasePropertyEditor* editor = CreatePropertyEditor(obj, p, cat);

		if (editor)
		{
			if (readOnly)
				editor->setEnabled(false);

			if (type->IsClass())
			{
				editor->SetDefaultObject(cdo);
				editor->Update();
				AddProperty(editor, (CObject*)obj, p);
			}

			cat->Widget()->layout()->addWidget(editor);
		}
	}

	if (type->IsClass() && bRecursive)
	{
		FClass* classType = (FClass*)type;

		if (classType->GetBaseClass())
			AddProperties(classType->GetBaseClass(), obj, cdo, true, overrideCat);
	}
}

void CPropertyEditorWidget::AddProperty(IBasePropertyEditor* editor, CObject* obj, const FProperty* field)
{
	properties.Add(editor);
	if (field && obj)
	{
		connect(editor, &IBasePropertyEditor::OnValueChanged, this, [=]() {
			if (field->meta)
			{
				const FFunction* f = obj->GetClass()->GetFunction(field->meta->FlagValue("ValidateFunc"));
				if (f)
				{
					gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
						FStack s(0);
						f->execFunc(obj, s);
					});
				}
			}

			if (auto* cmd = editor->ProvideUndoCmd(); cmd)
				gEditorWindow->sceneUndoStack->push(cmd);
		});
	}
}

//void CPropertyEditorWidget::propertyChanged(IBasePropertyEditor* pr)
//{
//	const FProperty* p = pr->;
//	if (p->meta)
//	{
//		const FFunction* f = type->GetFunction(p->meta->FlagValue("OnEditFunc"));
//		if (f)
//		{
//			gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
//				FStack s(0);
//				f->execFunc(obj, s);
//				//p->meta->onEditFunc(obj, s);
//				});
//		}
//	}
//}

IBasePropertyEditor::IBasePropertyEditor(QWidget* parent) : QWidget(parent)
{
}
