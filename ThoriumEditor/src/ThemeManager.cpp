
#include <string>
#include "ThemeManager.h"
#include "Resources/Asset.h"
#include "Resources/TextureAsset.h"
#include "Math/Math.h"
#include <Util/KeyValue.h>

static TMap<size_t, TObjectPtr<CTexture>> themeIcons;

// resource icons, can be either for a type or a specific object.
static TMap<size_t, TObjectPtr<CTexture>> resourceIcons;

static TArray<FEditorTheme> themes;
static size_t curTheme = 0;

const char* ImGuiColorNames[] = {
	"Text",
	"TextDisabled",
	"WindowBg",
	"ChildBg",
	"PopupBg",
	"Border",
	"BorderShadow",
	"FrameBg",
	"FrameBgHovered",
	"FrameBgActive",
	"TitleBg",
	"TitleBgActive",
	"TitleBgCollapsed",
	"MenuBarBg",
	"ScrollbarBg",
	"ScrollbarGrab",
	"ScrollbarGrabHovered",
	"ScrollbarGrabActive",
	"CheckMark",
	"SliderGrab",
	"SliderGrabActive",
	"Button",
	"ButtonHovered",
	"ButtonActive",
	"Header",
	"HeaderHovered",
	"HeaderActive",
	"Separator",
	"SeparatorHovered",
	"SeparatorActive",
	"ResizeGrip",
	"ResizeGripHovered",
	"ResizeGripActive",
	"Tab",
	"TabHovered",
	"TabActive",
	"TabUnfocused",
	"TabUnfocusedActive",
	"DockingPreview",
	"DockingEmptyBg",
	"PlotLines",
	"PlotLinesHovered",
	"PlotHistogram",
	"PlotHistogramHovered",
	"TableHeaderBg",
	"TableBorderStrong",
	"TableBorderLight",
	"TableRowBg",
	"TableRowBgAlt",
	"TextSelectedBg",
	"DragDropTarget",
	"NavHighlight",
	"NavWindowingHighlight",
	"NavWindowingDimBg",
	"ModalWindowDimBg",
};

static ImVec4 StringToVec4(const FString& str)
{
	TArray<FString> values = str.Split(',');

	ImVec4 r;
	for (int i = 0; i < FMath::Min(values.Size(), 4ull); i++)
	{
		FString value = values[i];
		value.EraseAll(' ');
		switch (i)
		{
		case 0:
			r.x = std::stof(value.c_str());
			break;
		case 1:
			r.y = std::stof(value.c_str());
			break;
		case 2:
			r.z = std::stof(value.c_str());
			break;
		case 3:
			r.w = std::stof(value.c_str());
			break;
		}
	}
	return r;
}

static ImVec2 StringToVec2(const FString& str)
{
	TArray<FString> values = str.Split(',');

	ImVec2 r;
	for (int i = 0; i < FMath::Min(values.Size(), 2ull); i++)
	{
		FString value = values[i];
		value.EraseAll(' ');
		switch (i)
		{
		case 0:
			r.x = std::stof(value.c_str());
			break;
		case 1:
			r.y = std::stof(value.c_str());
			break;
		}
	}
	return r;
}

static void LoadTheme(FFile* file)
{
	FKeyValue kv(file->FullPath());

	themes.Add();

	FEditorTheme& theme = *themes.last();
	theme.file = file;
	theme.name = *kv.GetValue("name");

	// set the default colors just in case
	memcpy(theme.colors, ImGui::GetStyle().Colors, sizeof(ImVec4) * ImGuiCol_COUNT);

	auto* colors = kv.GetCategory("Colors", true);

	for (int i = 0; i < ImGuiCol_COUNT; i++)
	{
		KVValue* v = colors->GetValue(ImGuiColorNames[i], false);
		if (!v)
			continue;

		theme.colors[i] = StringToVec4(*v);
	}

	theme.windowPadding = StringToVec2(*kv.GetValue("window_padding"));
	theme.framePadding = StringToVec2(*kv.GetValue("frame_padding"));
	theme.cellPadding = StringToVec2(*kv.GetValue("cell_padding"));
	theme.itemSpacing = StringToVec2(*kv.GetValue("item_spacing"));
	theme.windowRounding = std::stof(kv.GetValue("window_rounding")->Value.c_str());
	theme.childRounding = std::stof(kv.GetValue("child_rounding")->Value.c_str());
	theme.frameRounding = std::stof(kv.GetValue("frame_rounding")->Value.c_str());
	theme.grabRounding = std::stof(kv.GetValue("grab_rounding")->Value.c_str());
	theme.tabRounding = std::stof(kv.GetValue("tab_rounding")->Value.c_str());
}

static void LoadThemeIcons(const FString& themePath)
{
	FDirectory* iconsDir = CFileSystem::FindDirectory(themePath + "/icons");
	if (!iconsDir)
		return;

	for (auto* file : iconsDir->GetFiles())
	{
		bool bPng = file->Extension() == ".png";
		if (file->Extension() != ".thtex" && !bPng)
			continue;

		SizeType hash = file->Name().Hash();
		if (themeIcons.find(hash) != themeIcons.end())
			continue;

		TObjectPtr<CTexture> tex;
		if (bPng)
			tex = CTexture::CreateFromImage(ToFString(file->FullPath()));
		else
			tex = CResourceManager::GetResource<CTexture>(file->Path());

		if (tex)
			themeIcons[hash] = tex;
	}

	FDirectory* assetsDir = CFileSystem::FindDirectory(themePath + "/icons/assets");
	if (!assetsDir)
		return;

	for (auto* file : assetsDir->GetFiles())
	{
		bool bPng = file->Extension() == ".png";
		if (file->Extension() != ".thtex" && !bPng)
			continue;

		SizeType hash = file->Name().Hash();
		if (themeIcons.find(hash) != themeIcons.end())
			continue;

		TObjectPtr<CTexture> tex;
		if (bPng)
			tex = CTexture::CreateFromImage(ToFString(file->FullPath()));
		else
			tex = CResourceManager::GetResource<CTexture>(file->Path());

		if (tex)
			resourceIcons[hash] = tex;
	}
}

ITexture2D* ThoriumEditor::GetResourceIcon(CAsset* asset)
{
	// TODO: if this resource can have a unique icon we need to generate one.
	if (asset->GetClass() == CTexture::StaticClass())
	{
		SizeType pathHash = asset->File()->Path().Hash();
		auto it = resourceIcons.find(pathHash);
		if (it == resourceIcons.end())
		{
			resourceIcons[pathHash] = (CTexture*)asset;
			((CTexture*)asset)->Load(0);
			return (ITexture2D*)((CTexture*)asset)->GetTextureObject();
		}
		else
		{
			it->second->Load(0);
			return (ITexture2D*)it->second->GetTextureObject();
		}
	}

	return GetResourceIcon(asset->GetClass());
}

ITexture2D* ThoriumEditor::GetResourceIcon(FClass* type)
{
	auto it = resourceIcons.find(type->GetInternalName().Hash());
	if (it == resourceIcons.end())
		return nullptr;

	it->second->Load(0);
	return (ITexture2D*)it->second->GetTextureObject();
}

ITexture2D* ThoriumEditor::GetThemeIcon(const FString& iconName)
{
	auto it = themeIcons.find(iconName.Hash());
	if (it == themeIcons.end())
		return nullptr;

	it->second->Load(0);
	return (ITexture2D*)it->second->GetTextureObject();
}

void ThoriumEditor::ClearThemeIcons()
{
	themeIcons.clear();
	resourceIcons.clear();
}

void ThoriumEditor::LoadThemes()
{
	FDirectory* themesDir = CFileSystem::FindDirectory("editor/themes");
	if (!themesDir)
		return;

	LoadTheme(CFileSystem::FindFile("editor/themes/default/theme.cfg"));

	for (auto* dir : themesDir->GetSubDirectories())
	{
		if (dir->GetName() == "default")
			continue;

		FFile* themeFile = dir->GetFile("theme.cfg");
		if (!themeFile)
			continue;

		LoadTheme(themeFile);
	}
}

void ThoriumEditor::SetTheme(const FString& theme)
{
	FEditorTheme* t = nullptr;
	for (size_t i = 0; i < themes.Size(); i++)
	{
		if (themes[i].name == theme)
		{
			t = &themes[i];
			curTheme = i;
			break;
		}
	}

	if (!t)
		return;

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	memcpy(colors, t->colors, sizeof(ImVec4) * ImGuiCol_COUNT);

	style.WindowPadding = t->windowPadding;
	style.FramePadding = t->framePadding;
	style.CellPadding = t->cellPadding;
	style.ItemSpacing = t->itemSpacing;
	style.WindowRounding = t->windowRounding;
	style.ChildRounding = t->childRounding;
	style.FrameRounding = t->frameRounding;
	style.GrabRounding = t->grabRounding;
	style.TabRounding = t->tabRounding;

	themeIcons.clear();
	resourceIcons.clear();

	// Load theme icons
	FString themePath = t->file->Dir()->GetPath();
	
	LoadThemeIcons(themePath);

	// load default icons in case the current theme doesn't have them.
	if (t->name != "Default")
		LoadThemeIcons("editor/themes/default");
}

const TArray<FEditorTheme>& ThoriumEditor::GetThemes()
{
	return themes;
}

const FEditorTheme& ThoriumEditor::Theme()
{
	return themes[curTheme];
}

FEditorTheme& ThoriumEditor::AddTheme(const FString& name)
{
	themes.Add();

	FEditorTheme& th = *themes.last();
	// Copy the default theme
	th = themes[0];
	th.name = name;

	CFileSystem::FindMod("Engine")->CreateDir("editor/themes/" + name);

	SaveTheme(th);

	return th;
}

void ThoriumEditor::SaveTheme(const FEditorTheme& theme)
{
	FKeyValue kv(theme.file->FullPath());

	kv.SetValue("name", theme.name);

	auto* colors = kv.GetCategory("Colors", true);

	for (int i = 0; i < ImGuiCol_COUNT; i++)
	{
		FString value = (std::to_string(theme.colors[i].x) + 
			"," + std::to_string(theme.colors[i].y) +
			"," + std::to_string(theme.colors[i].z) +
			"," + std::to_string(theme.colors[i].w));

		colors->SetValue(ImGuiColorNames[i], value);
	}

	kv.SetValue("window_padding", std::to_string(theme.windowPadding.x) + "," + std::to_string(theme.windowPadding.y));
	kv.SetValue("frame_padding", std::to_string(theme.framePadding.x) + "," + std::to_string(theme.framePadding.y));
	kv.SetValue("cell_padding", std::to_string(theme.cellPadding.x) + "," + std::to_string(theme.cellPadding.y));
	kv.SetValue("item_spacing", std::to_string(theme.itemSpacing.x) + "," + std::to_string(theme.itemSpacing.y));
	kv.SetValue("window_rounding", std::to_string(theme.windowRounding));
	kv.SetValue("child_rounding", std::to_string(theme.childRounding));
	kv.SetValue("frame_rounding", std::to_string(theme.frameRounding));
	kv.SetValue("grab_rounding", std::to_string(theme.grabRounding));
	kv.SetValue("tab_rounding", std::to_string(theme.tabRounding));
}
