#pragma once

#include <Util/Core.h>
#include "Widgets/PropertyEditor.h"

struct FProperty;
struct FEnum;

class CStructProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CStructProperty(void* ptr, const FProperty* property, QWidget* parent = nullptr);
	CStructProperty(const FString& name, void* ptr, FStruct* type, QWidget* parent = nullptr);

	void Update() { for (auto* p : properties) p->Update(); }

private:
	TArray<IBasePropertyEditor*> properties;
	
};
