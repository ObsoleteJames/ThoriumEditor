
#include "ClassPtrProperty.h"
#include "Widgets/ClassSelectorWidget.h"
#include "EditorEngine.h"

#include <QMimeData>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QUndoCommand>

CClassPtrProperty::CClassPtrProperty(void* v, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value((FClass**)v)
{
	filter = CModuleManager::FindClass(property->typeName);

	Init(property->name.c_str());
}

CClassPtrProperty::CClassPtrProperty(const FString& name, void* v, FClass* clas, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value((FClass**)v), filter(clas)
{
	Init(name.c_str());
}

void CClassPtrProperty::Update()
{
	if (!value)
		return;

	FClass* c = *value;
	if (c != edit->GetClass())
		edit->SetClass(c);
}

void CClassPtrProperty::Init(const QString& name)
{
	setProperty("type", QVariant(1));
	auto* layout = new QHBoxLayout(this);
	const QString commandName = name + " Value Edited";

	QLabel* label = new QLabel(name, this);

	layout->addWidget(label);
	layout->addStretch(0);

	edit = new CClassSelectorWidget(filter, this);
	edit->setMinimumWidth(180);
	widget = edit;

	layout->addWidget(edit);

	connect(edit, &CClassSelectorWidget::ClassChanged, this, [=]() {
		gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
			class Undo : public QUndoCommand
			{
			public:
				Undo(const QString& name, FClass** ptr, FClass* oldv, FClass* newv)
					: QUndoCommand(name), ptr(ptr), oldValue(oldv), newValue(newv)
				{
				}

				int id() const override
				{
					return 1010;
				}

				bool mergeWith(const QUndoCommand* other) override
				{
					auto* cmd = static_cast<const Undo*>(other);
					if (!cmd || cmd->ptr != ptr)
						return false;
					newValue = cmd->newValue;
					return true;
				}

				void undo() override
				{
					*ptr = oldValue;
				}

				void redo() override
				{
					*ptr = newValue;
				}

				FClass** ptr;
				FClass* oldValue;
				FClass* newValue;
			};

			FClass* oldValue = *value;
			FClass* newValue = edit->GetClass();
			if (oldValue == newValue)
				return;
			curUndoCmd = new Undo(commandName, value, oldValue, newValue);
			*value = newValue;
			emit(OnValueChanged());
		});
	});

	Update();
}
