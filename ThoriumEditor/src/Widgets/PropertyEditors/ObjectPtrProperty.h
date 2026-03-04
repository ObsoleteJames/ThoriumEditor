#pragma once

#include <Util/Pointer.h>
#include "Widgets/PropertyEditor.h"
#include "Object/Object.h"
#include "Object/PropertyHandler.h"

class CObjectSelectorWidget;
class IPropertyHandler;
class QPushButton;

class CObjectPtrProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CObjectPtrProperty(void* value, const FProperty* property, QWidget* parent = nullptr);
	CObjectPtrProperty(const FString& name, void* value, FClass* clas, QWidget* parent = nullptr);

	void Update();

	void SetValue(void* value) { if (handler) { handler->SetValue(value); this->value = (TObjectPtr<CObject>*)handler->GetValue(); } else this->value = (TObjectPtr<CObject>*)value; Update(); }
	void AllowNull(bool b);

	inline CObjectSelectorWidget* GetSelector() const { return edit; }

private:
	void Init(const QString& name);

	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

	QUndoCommand* makeUndo(const TObjectPtr<CObject>& oldValue, const TObjectPtr<CObject>& newValue);
	void SetCurrentValue(const TObjectPtr<CObject>& newValue);
	TObjectPtr<CObject> GetCurrentValue() const;

private:
	CObjectSelectorWidget* edit;
	TObjectPtr<CObject>* value;
	bool bIsAsset;
	FClass* _class;
	QWidget* widget;

	QString undoName;
	mutable TUniquePtr<IPropertyHandler> handler;
	QPushButton* revertBtn = nullptr;

};
