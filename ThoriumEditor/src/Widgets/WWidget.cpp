
#include "WWidget.h"

void WWidget::AddWidget(WWidget* widget)
{
	if (widget->parent)
		return;

	childWidgets.Add(widget);
	widget->parent = this;
}

void WWidget::RemoveWidget(WWidget* widget)
{
	auto it = childWidgets.Find(widget);
	if (it != childWidgets.end())
	{
		childWidgets.Erase(it);
		widget->parent = nullptr;
	}
}

void WWidget::ClearChildren()
{
	for (auto w : childWidgets)
		w->parent = nullptr;

	childWidgets.Clear();
}

void WWidget::SaveSettings(KVCategory* kv)
{
}

void WWidget::LoadSettings(KVCategory* kv)
{
}

void WWidget::Render(BLContext*)
{
}

void WWidget::UpdateLayout()
{
	if (layout == EWidgetLayout::VERTICAL_STRETCH)
	{
		FVector2 childSize = rect.size;
		childSize.y = childSize.y / childWidgets.Size() - (paddingTop + paddingBottom + itemPadding);

		float y = rect.position.y + paddingTop;
		for (auto w : childWidgets)
		{
			w->rect.position = rect.position;
			w->rect.position.y = y;
			w->rect.size = childSize;

			y += childSize.y + itemPadding;
		}
	}
	if (layout == EWidgetLayout::HORIZONTAL_STRETCH)
	{
		FVector2 childSize = rect.size;
		childSize.x = childSize.x / childWidgets.Size() - (paddingLeft + paddingRight + itemPadding);

		float x = rect.position.x + paddingLeft;
		for (auto w : childWidgets)
		{
			w->rect.position = rect.position;
			w->rect.position.x = x;
			w->rect.size = childSize;

			x += childSize.x + itemPadding;
		}
	}
	if (layout == EWidgetLayout::VERTICAL)
	{
		float y = rect.position.y + paddingTop;
		for (auto w : childWidgets)
		{
			w->rect.position = rect.position;
			w->rect.position.y = y;
			w->rect.size.x = rect.size.x;

			y += w->rect.size.y + itemPadding;
		}
	}
	if (layout == EWidgetLayout::HORIZONTAL)
	{
		float x = rect.position.x + paddingLeft;
		for (auto w : childWidgets)
		{
			w->rect.position = rect.position;
			w->rect.position.x = x;
			w->rect.size.y = rect.size.y;

			x += w->rect.size.x + itemPadding;
		}
	}
}

void WWidget::OnDelete()
{

}

void WWidget::Update()
{

}
