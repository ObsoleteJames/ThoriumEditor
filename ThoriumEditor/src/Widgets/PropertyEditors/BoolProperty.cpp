
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
			Undo(const QString& name, bool* ptr, bool oldv, bool newv) : QUndoCommand(name), ptr(ptr), oldValue(oldv), newValue(newv)
			{
			}

			int id() const override
			{
				return 1001;
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

			bool* ptr;
			bool oldValue;
			bool newValue;
		};

		bool newValue = b != 0;
		if (*value == newValue)
			return;
		curUndoCmd = new Undo((property->name + " Value Edited").c_str(), value, *value, newValue);
		*value = newValue;
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
	const QString commandName = (name + " Value Edited").c_str();
	connect(editor, &QCheckBox::stateChanged, this, [=](int b) {
		class Undo : public QUndoCommand
		{
		public:
			Undo(const QString& name, bool* ptr, bool oldv, bool newv) : QUndoCommand(name), ptr(ptr), oldValue(oldv), newValue(newv)
			{
			}

			int id() const override
			{
				return 1001;
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

			bool* ptr;
			bool oldValue;
			bool newValue;
		};

		bool newValue = b != 0;
		if (*value == newValue)
			return;
		curUndoCmd = new Undo(commandName, value, *value, newValue);
		*value = newValue;
		emit(OnValueChanged());
	});
}

void CBoolProperty::Update()
{
	blockSignals(true);
	if (editor->isChecked() != *value)
		editor->setChecked(*value);
	blockSignals(false);
}
