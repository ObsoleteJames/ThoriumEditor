#pragma once

#include "EditorTool.h"
#include "DockWidget.h"

class CObjectTool : public IEditorTool
{
	Q_OBJECT

public:
	CObjectTool();

	void Init() final;

	void Enable() final;
	void Disable() final;

private:
	ads::CDockWidget* toolWindow;

};
