#pragma once

#include "WWidget.h"

#include "DockWidget.generated.h"

class WDockArea;

CLASS()
class SDK_API WDockWidget : public WWidget
{
	GENERATED_BODY()

	friend class WDockArea;

public:
	inline WDockArea* GetDockArea() const { return dockedArea; }


private:
	WDockArea* dockedArea = nullptr;

};