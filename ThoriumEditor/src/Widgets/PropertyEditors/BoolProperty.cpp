
#include "BoolProperty.h"
#include "Object/Class.h"

#include <QLabel>
#include <QCheckBox>
#include <QBoxLayout>
#include <QVariant>
#include <QUndoCommand>

CBoolProperty::CBoolProperty(bool* v, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value(v)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());

	editor = new QCheckBox(this);

	QLabel* label = new QLabel(property->name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	Update();
	connect(editor, &QCheckBox::stateChanged, this, [=](int b) {
		class Undo : public QUndoCommand
		{
		public:
			Undo(const QString& name, CBoolProperty* edit, bool* ptr, bool oldv, bool newv) : QUndoCommand(name), ptr(ptr), oldValue(oldv), newValue(newv)
			{
			}

			void undo() override
			{
				*ptr = oldValue;
			}

			void redo() override
			{
				*ptr = newValue;
			}

			bool* ptr;
			bool oldValue;
			bool newValue;
		};

		curUndoCmd = new Undo((property->name + " Value Edited").c_str(), this, value, *value, b);
		*value = b;
		emit(OnValueChanged());
	});
}

CBoolProperty::CBoolProperty(const FString& name, bool* v, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value(v)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());

	editor = new QCheckBox(this);

	QLabel* label = new QLabel(name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	Update();
	connect(editor, &QCheckBox::stateChanged, this, [=](int b) { *value = b; emit(OnValueChanged()); });
}

void CBoolProperty::Update()
{
	blockSignals(true);
	if (editor->isChecked() != *value)
		editor->setChecked(*value);
	blockSignals(false);
}
