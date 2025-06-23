
#include "WidgetManager.h"
#include "WWidget.h"

//#include "blend2d.h"

void CWidgetManager::UpdateWidget(WWidget* widget)
{
	auto it = updateWidgets.Find(widget);
	if (it == updateWidgets.end())
		updateWidgets.Add(widget);
}

void CWidgetManager::Init()
{

}

void CWidgetManager::Update()
{
	for (auto w : updateWidgets)
		w->UpdateLayout();
	
	updateWidgets.Clear();
}

void CWidgetManager::Render(WWidget* widget)
{
	
}

void CWidgetManager::Render(WWidget* widget, BLContext* ctx)
{

}
