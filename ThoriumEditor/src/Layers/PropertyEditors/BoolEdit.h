#pragma once

#include "PropertyEditor.h"

class CBoolEdit : public IPropertyEditor
{
public:
	CBoolEdit(int numObjects, CObject** object, void** data, const FProperty* property);

	void Render() override;

};
