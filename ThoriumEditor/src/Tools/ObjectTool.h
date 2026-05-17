#pragma once

#include "EditorTool.h"
#include "DockWidget.h"
#include "Object/Object.h"

class FGizmo;
//class PObjectTool;

class CObjectTool : public IEditorTool
{
	Q_OBJECT

	//friend class PObjectTool;

public:
	CObjectTool();

	void Init() final;

	void Enable() final;
	void Disable() final;

	void Update() final;

private:
	void GameUpdate();

	bool viewportEvent(QObject* obj, QEvent* ev) override;

private:
	ads::CDockWidget* toolWindow;

	TObjectPtr<FGizmo> gizmo;
	bool bSnapGizmo = false;
	bool bCtrl = false; // is CTRL being held

	bool bEnabled = false;

	// viewport data.
	QPointF mousePos;
	uint mouseBtns = 0;
	uint mouseModifiers = 0;

};
