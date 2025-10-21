#pragma once

#include "PropertyEditor.h"
#include "Util/Pointer.h"

class CArrayEdit : public IPropertyEditor
{
public:
	CArrayEdit(int numObjects, CObject** object, void** data, const FProperty* property);
	~CArrayEdit();

	void Render() override;

private:
	TArray<TUniquePtr<IPropertyEditor>> editors;

	FArrayHelper* arrData;
	FProperty* arrType;
};
