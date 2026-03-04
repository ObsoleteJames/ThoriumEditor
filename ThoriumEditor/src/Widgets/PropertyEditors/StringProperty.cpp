
#include <string>
#include "StringProperty.h"
#include "Object/Class.h"
#include "Object/PropertyHandler.h"

#include <QVariant>
#include <QLineEdit>
#include <QBoxLayout>
#include <QLabel>
#include <QUndoCommand>
#include <QPushButton>

class CStringValueUndo : public QUndoCommand
{
public:
	CStringValueUndo(const QString& name, FString* ptr, const FString& oldv, const FString& newv)
		: QUndoCommand(name), ptr(ptr), oldValue(oldv), newValue(newv)
	{
	}

	int id() const override
	{
		return 1005;
	}

	bool mergeWith(const QUndoCommand* other) override
	{
		auto* cmd = static_cast<const CStringValueUndo*>(other);
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

CStringProperty::CStringProperty(void* ptr, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{
	this->property = property;
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());
	undoName = property->name;

	handler = property->GetHandler(ptr);
	fstring = (FString*)handler->GetValue();

	editor = new QLineEdit(this);

	QLabel* label = new QLabel(property->name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	revertBtn = AddRevertBtn(handler);
	layout()->addWidget(revertBtn);
	connect(revertBtn, &QPushButton::clicked, this, [=]() {
		if (!cdo || !this->property || !handler)
			return;

		FString* newValue = (FString*)((SizeType)cdo + this->property->offset);
		FString oldValue = handler->GetValue<FString>();
		if (oldValue == *newValue)
			return;

		curUndoCmd = new CStringValueUndo((undoName + " Value Edited").c_str(), (FString*)handler->GetValue(), oldValue, *newValue);
		handler->SetValue(newValue);
		emit(OnValueChanged());
		Update();
	});

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

	revertBtn = AddRevertBtn(nullptr);
	layout()->addWidget(revertBtn);
	revertBtn->setVisible(false);

	Update();
	connect(editor, &QLineEdit::textEdited, this, &CStringProperty::onEdit);
}

void CStringProperty::Update()
{
	if (handler)
	{
		fstring = (FString*)handler->GetValue();
	}

	if (editor->text() != fstring->c_str())
		editor->setText(fstring->c_str());

	if (cdo && this->property && handler && revertBtn)
	{
		bool b = handler->Equals((void*)((SizeType)cdo + this->property->offset));
		revertBtn->setVisible(!b);
	}
}

void CStringProperty::onEdit(const QString& txt)
{
	FString newValue = txt.toStdString();
	FString oldValue = handler ? handler->GetValue<FString>() : *fstring;
	if (oldValue == newValue)
		return;
	curUndoCmd = new CStringValueUndo((undoName + " Value Edited").c_str(), handler ? (FString*)handler->GetValue() : fstring, oldValue, newValue);
	if (handler)
		handler->SetValue(&newValue);
	else
		*fstring = newValue;
	emit(OnValueChanged());
}
