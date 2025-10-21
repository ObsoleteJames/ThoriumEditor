#pragma once

#include "EditorTool.h"
#include "DockWidget.h"

class CModellingTool : public IEditorTool
{
	Q_OBJECT

public:
	CModellingTool();

	void Init() final;

	void Enable() final;
	void Disable() final;

private:
	ads::CDockWidget* toolWindow;

};
