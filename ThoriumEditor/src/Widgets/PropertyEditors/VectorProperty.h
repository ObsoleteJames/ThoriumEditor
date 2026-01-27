#pragma once

#include "Widgets/PropertyEditor.h"
#include "Math/Vectors.h"

class QDoubleSpinBox;

class CVectorProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CVectorProperty(FVector* value, const FString& name, QWidget* parent = nullptr);

	void Update();

	inline void SetValue(FVector* v) { value = (float*)v; Update(); }

private:
	QDoubleSpinBox* editors[3];
	float* value;
};
