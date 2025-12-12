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

private:
	ads::CDockWidget* toolWindow;

	TObjectPtr<FGizmo> gizmo;

	//PObjectTool* p;
};
