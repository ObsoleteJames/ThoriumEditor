#pragma once

#include "PropertyEditor.h"
#include "Util/Pointer.h"

class CColor4Edit : public IPropertyEditor
{
public:
	CColor4Edit(int numObjects, CObject** object, void** data, const FProperty* property);

	void Render() override;

private:
	TArray<TUniquePtr<IPropertyEditor>> editors;
};

class CColor3Edit : public IPropertyEditor
{
public:
	CColor3Edit(int numObjects, CObject** object, void** data, const FProperty* property);

	void Render() override;

private:
	TArray<TUniquePtr<IPropertyEditor>> editors;
};
