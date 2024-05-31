#pragma once

#include "Layer.h"
#include "EditorCore.h"

class CModule;

class CModuleDebugger : public CLayer
{
public:
	void OnUIRender() override;

	void SetActive(FAssetClass* ptr);
	void SetActive(FStruct* ptr);
	void SetActive(FEnum* ptr);

private:
	CModule* selected;
	
	FAssetClass* activeAsset = nullptr;
	FStruct* activeStruct = nullptr;
	FEnum* activeEnum = nullptr;
};
