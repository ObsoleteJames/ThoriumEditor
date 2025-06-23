#pragma once

#include "EditorCore.h"
#include <Util/Array.h>

class CWidgetManager;
class WWidget;
class BLContext;

extern CWidgetManager* gWidgetManager;

class SDK_API CWidgetManager
{
public:
	void UpdateWidget(WWidget* widget);

	void Init();

	void Update();
	void Render(WWidget* widget);

private:
	void Render(WWidget* widget, BLContext* ctx);

private:
	TArray<WWidget*> updateWidgets;
};
