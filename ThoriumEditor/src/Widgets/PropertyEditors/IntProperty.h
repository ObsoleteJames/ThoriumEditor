#pragma once

#include <Util/Core.h>
#include <Util/Pointer.h>
#include "Widgets/PropertyEditor.h"
#include "Object/PropertyHandler.h"

struct FProperty;
class QSpinBox;
class IPropertyHandler;
class QPushButton;

class CIntProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CIntProperty(int* value, const FProperty* property, QWidget* parent = nullptr);
	CIntProperty(const FString& name, int* value, int min = 0, int max = 0, QWidget* parent = nullptr);

	void Update();

	void SetValue(int* value) { if (handler) handler->SetValue(value); else this->value = value; Update(); }

public Q_SLOTS:
	void onValueChanged(int);

private:
	QSpinBox* editor;

	uint8 byteSize = 4;
	int* value;
	FString undoName;
	TUniquePtr<IPropertyHandler> handler;
	QPushButton* revertBtn = nullptr;

};

class CUIntProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CUIntProperty(uint* value, const FProperty* property, QWidget* parent = nullptr);
	CUIntProperty(const FString& name, uint* value, uint min = 0, uint max = 0, QWidget* parent = nullptr);

	void Update();

	void SetValue(uint* value) { if (handler) handler->SetValue(value); else this->value = value; Update(); }

public Q_SLOTS:
	void onValueChanged(int);

private:
	QSpinBox* editor;

	uint8 byteSize = 4;
	uint* value;
	FString undoName;
	TUniquePtr<IPropertyHandler> handler;
	QPushButton* revertBtn = nullptr;

};
