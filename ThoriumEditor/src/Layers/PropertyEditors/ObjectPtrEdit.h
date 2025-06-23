#pragma once

#include "PropertyEditor.h"

class CObjectPtrEdit : public IPropertyEditor
{
public:
	CObjectPtrEdit(int numObjects, CObject** object, void** data, const FProperty* property);

	void Render() override;

};
