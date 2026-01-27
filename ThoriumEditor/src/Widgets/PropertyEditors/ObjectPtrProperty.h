#pragma once

#include "Widgets/PropertyEditor.h"
#include "Object/Object.h"

class CObjectSelectorWidget;

class CObjectPtrProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CObjectPtrProperty(void* value, const FProperty* property, QWidget* parent = nullptr);
	CObjectPtrProperty(const FString& name, void* value, FClass* clas, QWidget* parent = nullptr);

	void Update();

	void SetValue(void* value) { this->value = (TObjectPtr<CObject>*)value; Update(); }
	void AllowNull(bool b);

	inline CObjectSelectorWidget* GetSelector() const { return edit; }

private:
	void Init(const QString& name);

	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

private:
	CObjectSelectorWidget* edit;
	TObjectPtr<CObject>* value;
	bool bIsAsset;
	FClass* _class;
	QWidget* widget;

};
