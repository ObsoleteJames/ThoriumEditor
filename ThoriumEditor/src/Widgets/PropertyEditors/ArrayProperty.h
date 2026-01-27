#pragma once

#include <Util/Core.h>
#include "Widgets/PropertyEditor.h"

struct FProperty;
struct FArrayHelper;

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
	FArrayHelper* helper;
	QWidget* content;
	TArray<IBasePropertyEditor*> editors;
	FString typeName;

};
