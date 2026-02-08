#pragma once

#include <Util/Core.h>
#include "Widgets/PropertyEditor.h"

struct FProperty;
class QLineEdit;

class CStringProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CStringProperty(void* ptr, const FProperty* property, QWidget* parent = nullptr);
	CStringProperty(const FString& name, FString* ptr, QWidget* parent = nullptr);

	void Update();

	void SetValue(FString* ptr) { fstring = ptr; Update(); }

private:
	void onEdit(const QString&);

private:
	QLineEdit* editor;
	FString* fstring;
	FString undoName;

};
