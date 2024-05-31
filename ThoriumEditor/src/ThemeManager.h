#pragma once

#include "EngineCore.h"
#include "ImGui/imgui.h"

class ITexture2D;
class CAsset;
class FClass;
struct FFile;

struct FEditorTheme
{
public:
	FString name;

	FFile* file;

	ImVec4 colors[ImGuiCol_COUNT];
	ImVec2 windowPadding;
	ImVec2 framePadding;
	ImVec2 cellPadding;
	ImVec2 itemSpacing;
	float windowRounding;
	float childRounding;
	float frameRounding;
	float grabRounding;
	float tabRounding;
};

namespace ThoriumEditor
{
	ITexture2D* GetResourceIcon(CAsset* asset);
	ITexture2D* GetResourceIcon(FClass* type);
	ITexture2D* GetThemeIcon(const FString& iconName);

	void ClearThemeIcons();
	void LoadThemes();

	void SetTheme(const FString& theme);
	const TArray<FEditorTheme>& GetThemes();
	const FEditorTheme& Theme();
	FEditorTheme& AddTheme(const FString& name);
	void SaveTheme(const FEditorTheme& theme);
}
