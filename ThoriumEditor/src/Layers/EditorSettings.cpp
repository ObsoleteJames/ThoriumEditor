
#include "EditorSettings.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "EditorWidgets.h"
#include "EditorEngine.h"
#include "EditorMenu.h"

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
