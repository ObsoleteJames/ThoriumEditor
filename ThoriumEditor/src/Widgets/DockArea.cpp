
#include "DockArea.h"
#include "DockWidget.h"

void WDockArea::Dock(WDockWidget* widget)
{
	if (widget->GetDockArea())
		return;

	if (IsSplit())
	{
		GetChildArea(0)->Dock(widget);
		return;
	}

	dockedWidgets.Add(widget);
	widget->dockedArea = this;
}

void WDockArea::Release(WDockWidget* widget)
{
	auto it = dockedWidgets.Find(widget);
	if (it != dockedWidgets.end())
	{
		dockedWidgets.Erase(it);
		widget->dockedArea = nullptr;
	}
}

void WDockArea::SetActive(WDockWidget* widget)
{
	auto it = dockedWidgets.Find(widget);
	if (it != dockedWidgets.end())
		activeWidget = dockedWidgets.Index(it);
}

void WDockArea::SetActive(int index)
{
	activeWidget = index < dockedWidgets.Size() ? index : 0;
}

void WDockArea::SplitVertical()
{
	if (IsSplit())
		return;

	children[0] = CreateObject<WDockArea>();
	children[0]->parentArea = this;
	children[1] = CreateObject<WDockArea>();
	children[1]->parentArea = this;

	bSplitVertical = true;
}

void WDockArea::SplitHorizontal()
{
	if (IsSplit())
		return;

	children[0] = CreateObject<WDockArea>();
	children[0]->parentArea = this;
	children[1] = CreateObject<WDockArea>();
	children[1]->parentArea = this;

	bSplitVertical = false;
}

void WDockArea::MergeChildren()
{
	if (!IsSplit())
		return;

	for (auto w : children[0]->dockedWidgets)
	{
		dockedWidgets.Add(w);
		w->dockedArea = this;
	}
	for (auto w : children[1]->dockedWidgets)
	{
		dockedWidgets.Add(w);
		w->dockedArea = this;
	}

	children[0]->Delete();
	children[0] = nullptr;
	
	children[1]->Delete();
	children[1] = nullptr;
}

void WDockArea::SaveSettings(KVCategory* kv)
{

}

void WDockArea::LoadSettings(KVCategory* kv)
{

}

void WDockArea::Render(BLContext* ctx)
{

}
