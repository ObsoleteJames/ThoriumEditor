#pragma once

#include "Layer.h"
#include "EngineCore.h"

class CEntity;

class CInputOutputWidget : public CLayer
{
public:
	void OnUIRender() override;

private:
	CEntity* prevEnt;
	int selectedOutput = -1;

};
