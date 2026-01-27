
#include "FloatProperty.h"
#include "Object/Class.h"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QBoxLayout>
#include <QVariant>

CFloatProperty::CFloatProperty(void* value, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());

	editor = new QDoubleSpinBox(this);

	if (property->type == EVT_FLOAT)
	{
		vFloat = (float*)value;
		bDouble = false;
	}
	else
	{
		vDouble = (double*)value;
		bDouble = true;
	}

	editor->setMinimum(FLT_MIN);
	editor->setMaximum(FLT_MAX);
	editor->setSingleStep(0.1);

	QLabel* label = new QLabel(property->name.c_str());

	layout()->addWidget(label);
	layout()->addWidget(editor);

	Update();
	connect(editor, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)));
}

CFloatProperty::CFloatProperty(const FString& name, float* value, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), vFloat(value), bDouble(false)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());

	editor = new QDoubleSpinBox(this);

	editor->setMinimum(FLT_MIN);
	editor->setMaximum(FLT_MAX);
	editor->setSingleStep(0.1);

	QLabel* label = new QLabel(name.c_str());

	layout()->addWidget(label);
	layout()->addWidget(editor);

	Update();
	connect(editor, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)));
}

CFloatProperty::CFloatProperty(const FString& name, double* value, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), vDouble(value), bDouble(true)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());

	editor = new QDoubleSpinBox(this);

	editor->setMinimum(FLT_MIN);
	editor->setMaximum(FLT_MAX);
	editor->setSingleStep(0.1);

	QLabel* label = new QLabel(name.c_str());

	layout()->addWidget(label);
	layout()->addWidget(editor);

	Update();
	connect(editor, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)));
}

void CFloatProperty::Update()
{
	if (bDouble)
	{
		if (editor->value() != *vDouble)
			editor->setValue(*vDouble);
	}
	else
	{
		if (editor->value() != *vFloat)
			editor->setValue(*vFloat);
	}
}

void CFloatProperty::valueChanged(double v)
{
	if (bDouble)
		*vDouble = v;
	else
		*vFloat = (float)v;
	emit(OnValueChanged());
}
