#pragma once

#include "WWidget.h"
#include "DockArea.generated.h"

class WDockWidget;

CLASS()
class SDK_API WDockArea : public WWidget
{
	GENERATED_BODY()

public:
	inline WDockArea* GetChildArea(int index = 0) const { return children[index]; }

	inline bool IsSplit() const { return children[0] != nullptr; }

public:
	void Dock(WDockWidget* widget);
	void Release(WDockWidget* widget);

	void SetActive(WDockWidget* widget);
	void SetActive(int index);

	void SplitVertical();
	void SplitHorizontal();
	void MergeChildren();

public:
	void SaveSettings(KVCategory* kv) override;
	void LoadSettings(KVCategory* kv) override;

	void Render(BLContext* ctx) override;

private:
	TObjectPtr<WDockArea> children[2] = {};
	TObjectPtr<WDockArea> parentArea;

	bool bSplitVertical = 0;

	int activeWidget = 0;
	TArray<TObjectPtr<WDockWidget>> dockedWidgets;
};
