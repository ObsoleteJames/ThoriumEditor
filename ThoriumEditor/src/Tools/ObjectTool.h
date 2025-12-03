#pragma once

#include "EditorTool.h"
#include "DockWidget.h"
#include "Object/Object.h"

class FGizmo;

class CObjectTool : public IEditorTool
{
	Q_OBJECT

public:
	CObjectTool();

	void Init() final;

	void Enable() final;
	void Disable() final;

	void Update() final;

private:
	ads::CDockWidget* toolWindow;

	TObjectPtr<FGizmo> gizmo;

};
