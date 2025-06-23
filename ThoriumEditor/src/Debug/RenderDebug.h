#pragma once

#include "Layer.h"
#include "EditorCore.h"

class CRenderDebugger : public CLayer
{
public:
	void OnUIRender() override;

	void TabScene();
	void TabLights();

};
