#pragma once

#include "Layer.h"
#include "EngineCore.h"

enum EEditorSettingsMenu : int
{
	ESM_GENERAL,
	ESM_SHORTCUTS,
	ESM_THEMES,
	ESM_ALL
};

struct FEditorShortcut;

class CEditorSettingsWidget : public CLayer
{
public:
	CEditorSettingsWidget();

	void OnUIRender() override;

	void SettingsGeneral();
	void SettingsShortcuts();
	void SettingsThemes();

	bool ShortcutEditor(const char* id, FEditorShortcut* sc);

public:
	int curMenu = ESM_GENERAL;
};
