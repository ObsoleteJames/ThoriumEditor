#pragma once

#include <QFrame>
#include <Util/Map.h>
#include "Object/Object.h"
#include "Widgets/TreeDataItem.h"

class QScrollArea;
class QVBoxLayout;
class CCollapsableWidget;
class QUndoCommand;

class IBasePropertyEditor : public QWidget
{
	Q_OBJECT

public:
	IBasePropertyEditor(QWidget* parent);

	virtual QWidget* GetWidget() { return this; }
	virtual void Update() = 0;

	// this will be called after OnValueChanged has been emitted.
	virtual QUndoCommand* ProvideUndoCmd();

Q_SIGNALS:
	void OnValueChanged();

protected:
	QUndoCommand* curUndoCmd = nullptr;

};

class CPropertyEditorWidget : public QWidget
{
	Q_OBJECT

public:
	CPropertyEditorWidget(QWidget* parent = nullptr);

	void SetObject(CObject* obj);
	inline TObjectPtr<CObject> GetObject() const { return targetObject; }

	inline void SetReadOnly(bool b) { bReadOnly = b; RebuildUI(); }
	inline bool ReadOnly() const { return bReadOnly; }

	static IBasePropertyEditor* CreatePropertyEditor(void* ptr, const FProperty* p, QWidget* parent);

	// Updates all property values;
	void Update();

private:
	void RebuildUI();

	CCollapsableWidget* GetCategoryWidget(const FString& category);

	void AddProperties(FClass* type, CObject* obj, const FString& overrideCat = FString());
	void AddProperty(IBasePropertyEditor* editor, CObject* obj = nullptr, const FProperty* field = nullptr);

protected:
	QScrollArea* scrollArea;
	QVBoxLayout* scrollLayout;
	TArray<CCollapsableWidget*> curWidgets;
	TArray<IBasePropertyEditor*> properties;
	TObjectPtr<CObject> targetObject;
	bool bReadOnly;

};
