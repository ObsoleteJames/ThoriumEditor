#pragma once

#include "Widgets/PropertyEditor.h"
#include "Math/Vectors.h"

class QDoubleSpinBox;

class CQuatProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CQuatProperty(FQuaternion* value, const FString& name, QWidget* parent = nullptr);

	void Update();

	inline void SetValue(FQuaternion* v) { value = v; Update(); }

private:
	void Changed();

private:
	QDoubleSpinBox* editors[3];
	FQuaternion* value;

	FQuaternion cache;

};
