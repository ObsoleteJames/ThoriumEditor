#pragma once

#include "Layer.h"
#include "EngineCore.h"

struct FAddon;

class CAddonsWindow : public CLayer
{
public:
	CAddonsWindow();

private:
	void OnUpdate(double dt) override;
	void OnUIRender() override;

	void DrawAddonList();
	void DrawAddonEditor();

private:
	FAddon* editingAddon = nullptr;

	//float sizeL = 250;
	//float sizeR;

};
