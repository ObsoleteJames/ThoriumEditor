#pragma once

#include <Util/Core.h>
#include <Util/Pointer.h>
#include "Widgets/PropertyEditor.h"
#include "Object/PropertyHandler.h"

struct FProperty;
class QLineEdit;
class IPropertyHandler;
class QPushButton;

class CStringProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CStringProperty(void* ptr, const FProperty* property, QWidget* parent = nullptr);
	CStringProperty(const FString& name, FString* ptr, QWidget* parent = nullptr);

	void Update();

	void SetValue(FString* ptr) { if (handler) handler->SetValue(ptr); else fstring = ptr; Update(); }

private:
	void onEdit(const QString&);

private:
	QLineEdit* editor;
	FString* fstring;
	FString undoName;
	TUniquePtr<IPropertyHandler> handler;
	QPushButton* revertBtn = nullptr;

};
