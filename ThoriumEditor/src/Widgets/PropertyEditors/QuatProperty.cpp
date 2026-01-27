
#include "QuatProperty.h"
#include <QLabel>
#include <QDoubleSpinBox>
#include <QBoxLayout>
#include <QVariant>

CQuatProperty::CQuatProperty(FQuaternion* v, const FString& name, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value(v)
{
	setProperty("type", QVariant(1));
	auto* layout = new QHBoxLayout(this);

	QLabel* label = new QLabel(name.c_str(), this);

	layout->addWidget(label);
	layout->addStretch(0);

	FVector euler = v->ToEuler();

	for (uint8 i = 0; i < 3; i++)
	{
		editors[i] = new QDoubleSpinBox(this);
		editors[i]->setMinimum(-99999999);
		editors[i]->setMaximum(FLT_MAX);
		
		editors[i]->setMinimumWidth(60);

		const char* names[] = {
			"X:",
			"Y:",
			"Z:"
		};

		QLabel* label = new QLabel(names[i], this);

		layout->addWidget(label);
		layout->addWidget(editors[i]);

		editors[i]->setValue(((float*)&euler)[i]);

		connect(editors[i], (void (QDoubleSpinBox::*)(double)) & QDoubleSpinBox::valueChanged, this, &CQuatProperty::Changed);
	}

	Update();
}

void CQuatProperty::Update()
{
	if (!value)
		return;

	if (cache != *value)
	{
		FVector euler = value->ToEuler().Degrees();

		editors[0]->setValue(euler.x);
		editors[1]->setValue(euler.y);
		editors[2]->setValue(euler.z);
		cache = *value;
	}
}

void CQuatProperty::Changed()
{
	FVector euler;

	euler.x = editors[0]->value();
	euler.y = editors[1]->value();
	euler.z = editors[2]->value();

	FQuaternion q = FQuaternion::EulerAngles(euler.Radians());
	cache = q;
	*value = q;
	emit(OnValueChanged());
}
