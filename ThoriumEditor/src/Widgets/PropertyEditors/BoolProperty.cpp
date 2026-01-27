
#include "BoolProperty.h"
#include "Object/Class.h"

#include <QLabel>
#include <QCheckBox>
#include <QBoxLayout>
#include <QVariant>

CBoolProperty::CBoolProperty(bool* v, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value(v)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());

	editor = new QCheckBox(this);

	QLabel* label = new QLabel(property->name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	Update();
	connect(editor, &QCheckBox::stateChanged, this, [=](int b) { *value = b; emit(OnValueChanged()); });
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
	if (editor->isChecked() != *value)
		editor->setChecked(*value);
}
