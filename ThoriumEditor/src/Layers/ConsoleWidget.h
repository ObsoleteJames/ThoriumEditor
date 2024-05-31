#pragma once

#include "Layer.h"

class CConsoleWidget : public CLayer
{
public:
	void OnUIRender() override;

public:
	bool bShowInfo = true;
	bool bShowWarnings = true;
	bool bShowErrors = true;

};
