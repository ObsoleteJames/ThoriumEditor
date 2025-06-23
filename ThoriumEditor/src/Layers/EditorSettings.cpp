
#include "EditorSettings.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "EditorWidgets.h"
#include "EditorEngine.h"
#include "EditorMenu.h"
#include "ThemeManager.h"
#include "Dialogs/InputTextDialog.h"

REGISTER_EDITOR_LAYER(CEditorSettingsWidget, "Edit/Editor Settings", "Settings", false, false)

struct FESMenu
{
	const char* name;
	void (CEditorSettingsWidget::*renderFunc)();
};

static const FESMenu menus[] = {
	{ "General", &CEditorSettingsWidget::SettingsGeneral },
	{ "Shortcuts", &CEditorSettingsWidget::SettingsShortcuts },
	{ "Themes", &CEditorSettingsWidget::SettingsThemes }
};

static constexpr size_t menusCount = IM_ARRAYSIZE(menus);

struct FShortcutList
{
	FString context;
	TArray<FEditorShortcut*> shortcuts;
};

static TArray<FShortcutList> shortcutList;

CEditorSettingsWidget::CEditorSettingsWidget()
{
	TMap<SizeType, FShortcutList*> listIndex;

	for (int i = 0; i < FEditorShortcut::GetShortcuts().Size(); i++)
	{
		auto& sc = FEditorShortcut::GetShortcuts()[i];

		SizeType cHash = sc->context.Hash();

		auto it = listIndex.find(cHash);
		if (it == listIndex.end())
		{
			shortcutList.Add();
			shortcutList.last()->shortcuts.Add(sc);
			shortcutList.last()->context = sc->context;

			listIndex[cHash] = &(*shortcutList.last());
		}
		else
			it->second->shortcuts.Add(sc);
	}
}

void CEditorSettingsWidget::OnUIRender()
{
	if (ImGui::Begin("Editor Settings", &bEnabled))
	{
		auto contentSize = ImGui::GetContentRegionAvail();

		if (ImGui::BeginChild("_esMenu", ImVec2(200, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding))
		{
			for (int i = 0; i < menusCount; i++)
			{
				if (ImGui::Selectable(menus[i].name, curMenu == i))
					curMenu = i;
			}

			ImGui::Separator();

			if (ImGui::Selectable("All Settings", curMenu == ESM_ALL))
				curMenu = ESM_ALL;
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
		if (ImGui::BeginChild("_psSettings"))
		{
			if (curMenu == ESM_ALL)
			{
				for (int i = 0; i < menusCount; i++)
					(this->*menus[i].renderFunc)();
			}
			else
				(this->*menus[curMenu].renderFunc)();
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}
	ImGui::End();
	Menu()->bChecked = bEnabled;
}

void CEditorSettingsWidget::SettingsGeneral()
{
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	ImGui::Text("General");
	ImGui::PopFont();


}

void CEditorSettingsWidget::SettingsShortcuts()
{
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	ImGui::Text("Shortcuts");
	ImGui::PopFont();

	if (ImGui::BeginTable("_esShortcuts", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg))
	{
		for (auto& list : shortcutList)
		{
			if (!ImGui::TableTreeHeader(list.context.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
				continue;

			for (auto& sc : list.shortcuts)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text(sc->name.c_str());
				ImGui::TableNextColumn();

				ShortcutEditor((sc->context + sc->name).c_str(), sc);
			}

			ImGui::TreePop();
		}

		ImGui::EndTable();
	}
}

void CEditorSettingsWidget::SettingsThemes()
{
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	ImGui::Text("Themes");
	ImGui::PopFont();

	const auto& themes = ThoriumEditor::GetThemes();
	const FEditorTheme& theme = ThoriumEditor::Theme();

	if (ImGui::BeginTable("_esThemes", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg))
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Theme");
		ImGui::TableNextColumn();

		// ImGuiCol_ChildBg
		ImGui::PopStyleColor();

		ImGuiStyle& style = ImGui::GetStyle();
		static ImGuiStyle ref_saved_style;

		static bool init = true;
		if (init)
			ref_saved_style = style;
		init = false;

		if (ImGui::BeginCombo("##themeSelector", theme.name.c_str()))
		{
			for (auto& th : themes)
			{
				if (ImGui::Selectable(th.name.c_str(), theme.name == th.name))
				{
					ThoriumEditor::SetTheme(th.name);
					ref_saved_style = style;
				}
			}

			ImGui::EndCombo();
		}

		ImGui::SameLine();
		if (ImGui::Button("Add"))
		{
			CInputTextDialog dialog("Create Theme", "Name");

			if (dialog.Exec())
			{
				auto& newTheme = ThoriumEditor::AddTheme(dialog.GetText());
				ThoriumEditor::SetTheme(newTheme.name);
			}
		}

		if (ImGui::TableTreeHeader("Edit##editThemeHeader", 0, true))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (ImGui::Button("Save"))
				ThoriumEditor::SaveTheme(*(FEditorTheme*)&theme);

			ImGui::BeginDisabled(theme.name == "default");

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Name");
			ImGui::TableNextColumn();
			ImGui::InputText("##themeNameEdit", (FString*)&theme.name);

			ImGui::EndDisabled();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Sizes");
			ImGui::SliderFloat2("WindowPadding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
			ImGui::SliderFloat2("FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
			ImGui::SliderFloat2("CellPadding", (float*)&style.CellPadding, 0.0f, 20.0f, "%.0f");
			ImGui::SliderFloat2("ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");

			ImGui::SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
			ImGui::SliderFloat("ChildRounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
			ImGui::SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
			ImGui::SliderFloat("GrabRounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
			ImGui::SliderFloat("TabRounding", &style.TabRounding, 0.0f, 12.0f, "%.0f");

			ImGui::SeparatorText("Colours");

			static ImGuiTextFilter filter;
			filter.Draw("Filter colours", ImGui::GetFontSize() * 16);

			static ImGuiColorEditFlags alpha_flags = 0;
			if (ImGui::RadioButton("Opaque", alpha_flags == ImGuiColorEditFlags_None)) { alpha_flags = ImGuiColorEditFlags_None; } ImGui::SameLine();
			if (ImGui::RadioButton("Alpha", alpha_flags == ImGuiColorEditFlags_AlphaPreview)) { alpha_flags = ImGuiColorEditFlags_AlphaPreview; } ImGui::SameLine();
			if (ImGui::RadioButton("Both", alpha_flags == ImGuiColorEditFlags_AlphaPreviewHalf)) { alpha_flags = ImGuiColorEditFlags_AlphaPreviewHalf; } ImGui::SameLine();

			//ImGui::BeginChild("##colors", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NavFlattened);
			ImGui::PushItemWidth(-160);
			for (int i = 0; i < ImGuiCol_COUNT; i++)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				const char* name = ImGui::GetStyleColorName(i);
				if (!filter.PassFilter(name))
					continue;
				ImGui::Text(name);

				ImGui::TableNextColumn();

				ImGui::PushID(i);
				ImGui::ColorEdit4("##color", (float*)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | alpha_flags);
				if (memcmp(&style.Colors[i], &ref_saved_style.Colors[i], sizeof(ImVec4)) != 0)
				{
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x); if (ImGui::Button("Revert")) { style.Colors[i] = ref_saved_style.Colors[i]; }
				}
				ImGui::PopID();
			}
			ImGui::PopItemWidth();

			ImGui::TreePop();
		}
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));

		ImGui::EndTable();
	}
}

bool CEditorSettingsWidget::ShortcutEditor(const char* str_id, FEditorShortcut * sc)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	const ImGuiStyle& style = ImGui::GetStyle();
	if (window->SkipItems)
		return false;

	const ImGuiID id = window->GetID(str_id);

	const ImVec2 content = ImGui::GetContentRegionAvail();
	const ImVec2 cursor = ImGui::GetCursorScreenPos();

	const ImVec2 frameSize = ImVec2(FMath::Min(170.f, content.x - 5.f), 24);

	const ImRect bb(cursor, cursor + frameSize);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	ImGuiItemFlags item_flags = (g.LastItemData.ID == id ? g.LastItemData.InFlags : g.CurrentItemFlags);

	const bool bHover = ImGui::ItemHoverable(bb, id, item_flags);

	FString previewString = sc->ToString();

	if (bHover)
	{
		bool mouseClicked = 0;
		bool mouseReleased = 0;

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left, id))
			mouseClicked = true;
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			mouseReleased = true;

		if (mouseClicked && g.ActiveId != id)
		{
			ImGui::SetActiveID(id, window);
		}
	}

	if (g.ActiveId == id)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ImGui::ClearActiveID();
			goto exitForEscape;
		}

		bool bCtrl = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);
		bool bShift = ImGui::IsKeyDown(ImGuiKey_LeftShift);

		previewString = "press a key...";

		if (bCtrl && bShift)
			previewString = "Ctrl+Shift+";
		else if (bCtrl && !bShift)
			previewString = "Ctrl+";
		else if (!bCtrl && bShift)
			previewString = "Shift+";

		for (int i = ImGuiKey_Keyboard_BEGIN; i < ImGuiKey_Keyboard_END; i++)
		{
			if (i >= ImGuiKey_LeftCtrl && i <= ImGuiKey_RightSuper)
				continue;

			if (ImGui::IsKeyPressed((ImGuiKey)i))
			{
				sc->SetKey((ImGuiKey)i, bShift, bCtrl);

				ImGui::ClearActiveID();
			}
		}
	exitForEscape:
		(void)0; // so the compiler doesn't throw an error.
	}

	const bool bActive = ImGui::IsItemActive();

	ImVec4 color = style.Colors[bHover ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg];
	if (bActive)
		color = color * ImVec4(0.8f, 0.8f, 0.8f, 1.f);
	ImGui::RenderFrame(cursor, cursor + frameSize, ImGui::ColorConvertFloat4ToU32(color), bActive, style.FrameRounding);

	ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[bActive ? ImGuiCol_TextDisabled : ImGuiCol_Text]);
	ImGui::RenderText(cursor + style.FramePadding, previewString.c_str());
	ImGui::PopStyleColor();

	return false;
}
