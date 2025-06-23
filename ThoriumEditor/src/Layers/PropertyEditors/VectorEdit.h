#pragma once

#include "PropertyEditor.h"
#include "Util/Pointer.h"

class CVectorEdit : public IPropertyEditor
{
public:
	CVectorEdit(int numObjects, CObject** object, void** data, const FProperty* property);

	void Render() override;

private:
	TArray<TUniquePtr<IPropertyEditor>> editors;
};
