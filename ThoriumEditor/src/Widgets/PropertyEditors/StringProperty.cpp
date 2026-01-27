
#include <string>
#include "StringProperty.h"
#include "Object/Class.h"

#include <QVariant>
#include <QLineEdit>
#include <QBoxLayout>
#include <QLabel>

CStringProperty::CStringProperty(void* ptr, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());

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
	*fstring = txt.toStdString();
	emit(OnValueChanged());
}
