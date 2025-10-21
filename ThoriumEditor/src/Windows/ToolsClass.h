#pragma once

#include "Editor.h"
#include <Util/Core.h>
#include <QIcon>

class CToolsWindow;
class CToolsWidget;

struct EDITOR_API FToolsClassBase
{
	const char* Name;
	const char* ToolBarPath;
	SizeType Id;
	QIcon* icon;
};

struct EDITOR_API FToolsWindowClass : public FToolsClassBase
{
	virtual CToolsWindow* Create() = 0;
};

struct EDITOR_API FToolsWidgetClass : public FToolsClassBase
{
	virtual CToolsWidget* Create(CToolsWindow* parent) = 0;

	SizeType WindowHash = 0;
	bool bShowOnStart;
};

//extern TArray<FToolsWindowClass*> _RegisteredWindows;
//extern TArray<FToolsWidgetClass*> _RegisteredWidgets;
