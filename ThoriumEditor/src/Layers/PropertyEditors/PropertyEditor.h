#pragma once

class CObject;
struct FProperty;

#include "Object/Class.h"

class IPropertyEditor
{
public:
	IPropertyEditor(int numObjects, CObject** object, void** data, const FProperty* property);

	virtual void Render() = 0;

	static IPropertyEditor* GetEditor(int numObjects, CObject** object, void** data, const FProperty* property);

protected:
	void Validate();

	bool IsVisible();
	bool CanEdit();

protected:
	FProperty* property;
	FClass* type;

	TArray<CObject*> owners;
	TArray<void*> objects;

	bool bHasEditCondition = false;
	bool bHasVisibleCondition = false;
};
