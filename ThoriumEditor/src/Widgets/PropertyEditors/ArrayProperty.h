#pragma once

#include <Util/Core.h>
#include <Util/Pointer.h>
#include "Widgets/PropertyEditor.h"

struct FProperty;
class FArrayPropertyHandler;

class CArrayProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CArrayProperty(void* ptr, const FProperty* property, QWidget* parent = nullptr);

	void Update();

private:
	void UpdateList();

private:
	void* obj;
	//FArrayHelper* helper;
	TUniquePtr<FArrayPropertyHandler> handler;

	QWidget* content;
	TArray<IBasePropertyEditor*> editors;
	FString typeName;

	const FProperty* property;

};
