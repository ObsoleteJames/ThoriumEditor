
#include "StructProperty.h"
#include "Widgets/CollapsableWidget.h"
#include "Object/Class.h"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QBoxLayout>
#include <QVariant>

CStructProperty::CStructProperty(void* ptr, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{
	setLayout(new QVBoxLayout());
	layout()->setContentsMargins(0, 0, 0, 0);

	FStruct* type = CModuleManager::FindStruct(property->typeName);
	THORIUM_ASSERT(type, "Failed to find struct type.");

	CCollapsableWidget* header = new CCollapsableWidget(property->name.c_str(), nullptr, this);
	header->SetHeaderType(CCollapsableWidget::NESTED_HEADER);
	QWidget* content = new QWidget(header);
	QVBoxLayout* cl = new QVBoxLayout(content);
	cl->setContentsMargins(4, 0, 0, 0);
	cl->setSpacing(0);
	header->SetWidget(content);
	layout()->addWidget(header);

	for (const FProperty* p = type->GetPropertyList(); p != nullptr; p = p->next)
	{
		if ((p->flags & VTAG_EDITOR_EDITABLE) == 0 && (p->flags & VTAG_EDITOR_VISIBLE) == 0)
			continue;

		bool readOnly = p->flags & VTAG_EDITOR_VISIBLE;

		void* pPtr = (void*)((SizeType)ptr + p->offset);

		IBasePropertyEditor* editor = CPropertyEditorWidget::CreatePropertyEditor(pPtr, p, header);

		if (editor)
		{
			if (readOnly)
				editor->setEnabled(false);

			cl->addWidget(editor);
			connect(editor, &IBasePropertyEditor::OnValueChanged, this, [=]() { emit(OnValueChanged()); });
		}
	}
}

CStructProperty::CStructProperty(const FString& name, void* ptr, FStruct* type, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{

}
