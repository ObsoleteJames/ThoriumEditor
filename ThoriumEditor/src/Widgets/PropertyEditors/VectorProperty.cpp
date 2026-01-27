
#include "VectorProperty.h"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QBoxLayout>
#include <QVariant>

CVectorProperty::CVectorProperty(FVector* v, const FString& name, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value((float*)v)
{
	setProperty("type", QVariant(1));
	auto* layout = new QHBoxLayout(this);

	QLabel* label = new QLabel(name.c_str(), this);

	layout->addWidget(label);
	layout->addStretch(0);

	for (uint8 i = 0; i < 3; i++)
	{
		editors[i] = new QDoubleSpinBox(this);
		editors[i]->setMinimum(-99999999);
		editors[i]->setMaximum(FLT_MAX);
		editors[i]->setSingleStep(0.1);

		editors[i]->setMinimumWidth(60);

		const char* names[] = {
			"X:",
			"Y:",
			"Z:"
		};

		QLabel* label = new QLabel(names[i], this);

		layout->addWidget(label);
		layout->addWidget(editors[i]);

		connect(editors[i], (void (QDoubleSpinBox::*)(double))&QDoubleSpinBox::valueChanged, this, [=](double v) { 
			value[i] = (float)v; emit(OnValueChanged()); 
		});
	}

	Update();
}

void CVectorProperty::Update()
{
	for (uint8 i = 0; i < 3; i++)
	{
		if (editors[i]->value() != value[i])
			editors[i]->setValue(value[i]);
	}
}
