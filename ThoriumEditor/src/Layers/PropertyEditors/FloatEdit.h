#pragma once

#include "PropertyEditor.h"

class CFloatEdit : public IPropertyEditor
{
public:
	CFloatEdit(int numObjects, CObject** object, void** data, const FProperty* property);

	void Render() override;

};
