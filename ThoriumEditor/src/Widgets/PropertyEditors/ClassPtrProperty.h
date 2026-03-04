#pragma once

#include <Util/Pointer.h>
#include "Widgets/PropertyEditor.h"
#include "Object/Object.h"
#include "Object/PropertyHandler.h"

class CClassSelectorWidget;

class CClassPtrProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CClassPtrProperty(void* value, const FProperty* property, QWidget* parent = nullptr);
	CClassPtrProperty(const FString& name, void* value, FClass* filter, QWidget* parent = nullptr);

	void Update();

	void SetValue(void* value) { if (handler) handler->SetValue(value); Update(); }
	
	inline CClassSelectorWidget* GetSelector() const { return edit; }

private:
	void Init(const QString& name);

private:
	CClassSelectorWidget* edit;
	FClass* filter;
	QWidget* widget;

	TUniquePtr<IPropertyHandler> handler;
	QPushButton* revertBtn;
};
