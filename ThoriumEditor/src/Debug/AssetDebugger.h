#pragma once

#include "Layer.h"
#include "EditorCore.h"

class CModule;

class CAssetDebugger : public CLayer
{
public:
	void OnUIRender() override;

private:
	SizeType selected = -1;
};
