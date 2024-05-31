#pragma once

#include "Layer.h"
#include "EngineCore.h"

class CProjectSettingsWidget : public CLayer
{
public:
	void OnUIRender() override;

public:
	void RenderGameSettings();
	void RenderAudioSettings();
	void RenderInputSettings();
	void RenderPhysicsSettings();
	void RenderRenderSettings();

	bool QualitySetting(const char* label, int* value, int min, int max);

private:
	int curMenu = 0;

};
