#pragma once

#include "DockWidget.h"
#include <Util/Map.h>
#include "Object/Object.h"
#include "Widgets/PropertyEditor.h"

class CPropertyEditorWidget;
class QTreeWidgetItem;
class QSplitter;
class CSceneComponent;
class QLineEdit;
class QFrame;

class CPropertiesWidget : public ads::CDockWidget
{
	Q_OBJECT

	friend class CEditorWindow;

public:
	CPropertiesWidget(QWidget* parent = nullptr);
	virtual ~CPropertiesWidget();

	void SetObject(CObject* obj);
	inline TObjectPtr<CObject> GetObject() const { return targetObject; }

	inline void SetReadOnly(bool b) { bReadOnly = b; UpdateUI(); }
	inline bool ReadOnly() const { return bReadOnly; }

	void Update();

private:
	void UpdateUI();

	void AddComponent();

Q_SIGNALS:
	void ComponentSelected(TObjectPtr<CObject> obj);

private:
	CPropertyEditorWidget* editor;
	QTreeWidget* childTree;
	QWidget* topWidget;
	QSplitter* split;
	QLineEdit* nameEdit;

	TObjectPtr<CObject> targetObject;
	TObjectPtr<CObject> selectedChild;

	TMap<TObjectPtr<CSceneComponent>, QTreeWidgetItem*> sceneComps;
	TArray<QTreeWidgetItem*> items;

	bool bReadOnly;

};
