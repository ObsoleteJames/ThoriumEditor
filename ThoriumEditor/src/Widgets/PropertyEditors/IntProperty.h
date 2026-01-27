#pragma once

#include <Util/Core.h>
#include "Widgets/PropertyEditor.h"

struct FProperty;
class QSpinBox;

class CIntProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CIntProperty(int* value, const FProperty* property, QWidget* parent = nullptr);
	CIntProperty(const FString& name, int* value, int min = 0, int max = 0, QWidget* parent = nullptr);

	void Update();

	void SetValue(int* value) { this->value = value; Update(); }

public Q_SLOTS:
	void onValueChanged(int);

private:
	QSpinBox* editor;

	uint8 byteSize = 4;
	int* value;

};

class CUIntProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CUIntProperty(uint* value, const FProperty* property, QWidget* parent = nullptr);
	CUIntProperty(const FString& name, uint* value, uint min = 0, uint max = 0, QWidget* parent = nullptr);

	void Update();

	void SetValue(uint* value) { this->value = value; Update(); }

public Q_SLOTS:
	void onValueChanged(int);

private:
	QSpinBox* editor;

	uint8 byteSize = 4;
	uint* value;

};
