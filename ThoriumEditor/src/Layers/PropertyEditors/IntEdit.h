#pragma once

#include "PropertyEditor.h"

class CIntEdit : public IPropertyEditor
{
public:
	CIntEdit(int numObjects, CObject** object, void** data, const FProperty* property);

	void Render() override;

};
