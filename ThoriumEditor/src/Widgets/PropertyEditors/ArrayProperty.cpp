
#include "ArrayProperty.h"
#include "Widgets/CollapsableWidget.h"
#include "Object/Class.h"
#include "EditorEngine.h"

#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QBoxLayout>
#include <QVariant>
#include <QUndoCommand>

CArrayProperty::CArrayProperty(void* ptr, const FProperty* p, QWidget* parent) : IBasePropertyEditor(parent), property(p)
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

			class Undo : public QUndoCommand
			{
			public:
				Undo(const QString& name, void* obj, const FProperty* prop, int arrSize) : QUndoCommand(name)
				{
					index = arrSize - 1;
					property = prop;
					this->obj = obj;
				}

				int id() const override
				{
					return 1011;
				}

				bool mergeWith(const QUndoCommand* other) override
				{
					(void)other;
					return false;
				}

				void undo() final
				{
					auto* helper = (FArrayHelper*)property->typeHelper;
					helper->Erase(obj, index);
				}

				void redo() final
				{
					auto* helper = (FArrayHelper*)property->typeHelper;
					helper->AddEmpty(obj);
					index = helper->Size(obj) - 1;
				}

			private:
				int index;
				const FProperty* property;
				void* obj;
			};
			curUndoCmd = new Undo((property->name + " Add Item").c_str(), obj, property, helper->Size(obj));

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

	// TODO: fix this, crashes or freezes when updating list.

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

					class Undo : public QUndoCommand
					{
					public:
						Undo(const QString& name, void* obj, const FProperty* prop, int arrSize) : QUndoCommand(name)
						{
							index = arrSize - 1;
							property = prop;
							this->obj = obj;

							auto* helper = (FArrayHelper*)property->typeHelper;
							data = malloc(helper->objSize);
							memcpy(data, (void*)((SizeType)helper->Data(obj) + index * helper->objSize), helper->objSize);
						}

						int id() const override
						{
							return 1012;
						}

						bool mergeWith(const QUndoCommand* other) override
						{
							(void)other;
							return false;
						}
						virtual ~Undo() { free(data); }

						void undo() final
						{
							auto* helper = (FArrayHelper*)property->typeHelper;
							helper->AddEmpty(obj);
							index = helper->Size(obj) - 1;
							memcpy((void*)((SizeType)helper->Data(obj) + index * helper->objSize), data, helper->objSize);
						}

						void redo() final
						{
							auto* helper = (FArrayHelper*)property->typeHelper;
							helper->Erase(obj, index);
						}

					private:
						int index;
						const FProperty* property;
						void* obj;
						void* data;
					};
					curUndoCmd = new Undo((property->name + " Add Item").c_str(), obj, property, helper->Size(obj));

					emit(OnValueChanged());
					UpdateList();
				});
			});
			connect(editor, &IBasePropertyEditor::OnValueChanged, this, [=]() {
				if (auto* cmd = editor->ProvideUndoCmd(); cmd)
					curUndoCmd = cmd;

				emit this->OnValueChanged();
			});
		}
	}
}
