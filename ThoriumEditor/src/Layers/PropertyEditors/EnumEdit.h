#pragma once

#include "PropertyEditor.h"

class CEnumEdit : public IPropertyEditor
{
public:
	CEnumEdit(int numObjects, CObject** object, void** data, const FProperty* property);

	void Render() override;

};
