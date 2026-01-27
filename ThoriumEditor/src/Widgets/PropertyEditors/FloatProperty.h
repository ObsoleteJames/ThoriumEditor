#pragma once

#include <Util/Core.h>
#include "Widgets/PropertyEditor.h"

struct FProperty;
class QDoubleSpinBox;

class CFloatProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CFloatProperty(void* value, const FProperty* property, QWidget* parent = nullptr);
	CFloatProperty(const FString& name, float* value, QWidget* parent = nullptr);
	CFloatProperty(const FString& name, double* value, QWidget* parent = nullptr);

	void Update();

	void SetValue(float* value) { vFloat = value; bDouble = false; Update(); }
	void SetValue(double* value) { vDouble = value; bDouble = true; Update(); }

private Q_SLOTS:
	void valueChanged(double);

private:
	QDoubleSpinBox* editor;
	bool bDouble;
	union
	{
		float* vFloat;
		double* vDouble;
	};

};
