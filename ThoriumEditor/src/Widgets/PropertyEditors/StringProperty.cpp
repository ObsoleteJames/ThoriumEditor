
#include <string>
#include "StringProperty.h"
#include "Object/Class.h"

#include <QVariant>
#include <QLineEdit>
#include <QBoxLayout>
#include <QLabel>
#include <QUndoCommand>

CStringProperty::CStringProperty(void* ptr, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());
	undoName = property->name;

	fstring = (FString*)ptr;

	editor = new QLineEdit(this);

	QLabel* label = new QLabel(property->name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	Update();
	connect(editor, &QLineEdit::textEdited, this, &CStringProperty::onEdit);
}

CStringProperty::CStringProperty(const FString& name, FString* ptr, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());
	undoName = name;

	editor = new QLineEdit(this);

	fstring = ptr;

	QLabel* label = new QLabel(name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	Update();
	connect(editor, &QLineEdit::textEdited, this, &CStringProperty::onEdit);
}

void CStringProperty::Update()
{
	if (editor->text() != fstring->c_str())
		editor->setText(fstring->c_str());
}

void CStringProperty::onEdit(const QString& txt)
{
	class Undo : public QUndoCommand
	{
	public:
		Undo(const QString& name, FString* ptr, const FString& oldv, const FString& newv)
			: QUndoCommand(name), ptr(ptr), oldValue(oldv), newValue(newv)
		{
		}

		int id() const override
		{
			return 1005;
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

		FString* ptr;
		FString oldValue;
		FString newValue;
	};

	FString newValue = txt.toStdString();
	if (*fstring == newValue)
		return;
	curUndoCmd = new Undo((undoName + " Value Edited").c_str(), fstring, *fstring, newValue);
	*fstring = newValue;
	emit(OnValueChanged());
}
