#pragma once

#include <Util/Core.h>
#include "Widgets/PropertyEditor.h"

struct FProperty;
class QCheckBox;

class CBoolProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CBoolProperty(bool* value, const FProperty* property, QWidget* parent = nullptr);
	CBoolProperty(const FString& name, bool* value, QWidget* parent = nullptr);
	
	void Update();

	void SetValue(bool* value) { this->value = value; Update(); }

private:
	QCheckBox* editor;
	bool* value;

};
