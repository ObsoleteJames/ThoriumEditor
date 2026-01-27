
#include "ArrayProperty.h"
#include "Widgets/CollapsableWidget.h"
#include "Object/Class.h"
#include "EditorEngine.h"

#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QBoxLayout>
#include <QVariant>

CArrayProperty::CArrayProperty(void* ptr, const FProperty* property, QWidget* parent) : IBasePropertyEditor(parent)
{
	setLayout(new QVBoxLayout());
	layout()->setContentsMargins(0, 0, 0, 0);

	obj = ptr;
	helper = (FArrayHelper*)property->typeHelper;
	typeName = property->typeName;

	CCollapsableWidget* header = new CCollapsableWidget(property->name.c_str(), nullptr, this);
	header->SetHeaderType(CCollapsableWidget::NESTED_HEADER);
	header->GetHeader()->setMinimumHeight(32);
	content = new QWidget(header);
	QVBoxLayout* cl = new QVBoxLayout(content);
	cl->setContentsMargins(16, 0, 0, 0);
	cl->setSpacing(0);
	header->SetWidget(content);
	layout()->addWidget(header);

	QHBoxLayout* headerLayout = new QHBoxLayout(header->GetHeader());
	headerLayout->setContentsMargins(0, 0, 0, 0);

	QPushButton* btnAdd = new QPushButton("+", this);
	btnAdd->setProperty("type", QVariant("clear"));
	headerLayout->addStretch(0);
	headerLayout->addWidget(btnAdd);

	connect(btnAdd, &QPushButton::clicked, this, [=]() { 
		gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
			helper->AddEmpty(obj); 
			emit(OnValueChanged()); 
			UpdateList();
		});
	});

	UpdateList();
}

void CArrayProperty::Update()
{
	if (helper->Size(obj) != editors.Size())
		UpdateList();
}

void CArrayProperty::UpdateList()
{
	for (auto* edit : editors)
	{
		content->layout()->removeWidget(edit);
	}
	editors.Clear();

	SizeType size = helper->Size(obj);
	SizeType data = (SizeType)helper->Data(obj);

	for (SizeType i = 0; i < size; i++)
	{
		void* ptr = (void*)(data + (i * helper->objSize));
		
		FProperty prop;
		prop.type = helper->objType;
		prop.typeName = typeName;
		prop.name = FString::ToString(i) + ":";
		prop.offset = (SizeType)ptr - data;
		prop.size = helper->objSize;

		IBasePropertyEditor* editor = CPropertyEditorWidget::CreatePropertyEditor(ptr, &prop, content);

		if (editor)
		{
			QPushButton* removeBtn = new QPushButton("X", editor);
			removeBtn->setProperty("type", QVariant("clear"));
			QWidget* wd = editor->GetWidget();
			if (!wd->layout())
			{
				QHBoxLayout* l = new QHBoxLayout(wd);
				l->setContentsMargins(4,4,4,4);
				l->addStretch(1);
			}

			wd->layout()->addWidget(removeBtn);

			content->layout()->addWidget(editor);
			editors.Add(editor);
			connect(removeBtn, &QPushButton::clicked, this, [=]() { 
				gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
					helper->Erase(obj, i);
					emit(OnValueChanged());
					UpdateList();
				});
			});
		}
	}
}
