#pragma once

#include "PropertyEditor.h"

class CStringEdit : public IPropertyEditor
{
public:
	CStringEdit(int numObjects, CObject** object, void** data, const FProperty* property);

	void Render() override;

};
