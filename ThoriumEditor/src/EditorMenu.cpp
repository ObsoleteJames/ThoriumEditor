
#include "EditorMenu.h"
#include <Util/Map.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

CEditorMenu::CEditorMenu(const FString& n, bool toggle /*= true*/)
{
	name = n;
	bToggle = toggle;
}

CEditorMenu::CEditorMenu(const FString& n, const FString& s, bool toggle /*= true*/)
{
	bToggle = toggle;
	name = n;
	shortcut = s;
}

CEditorMenu::CEditorMenu(const FString& n, const FString& c, const FString& s, bool toggle /*= true*/)
{
	bToggle = toggle;
	name = n;
	shortcut = s;
	category = c;
}

CEditorMenu::~CEditorMenu()
{
	for (auto* c : children)
		delete c;

	if (parent)
	{
		parent->children.Erase(parent->children.Find(this));
		parent = nullptr;
	}
}

void CEditorMenu::SetParent(CEditorMenu* p)
{
	if (parent)
		parent->children.Erase(parent->children.Find(this));
	
	parent = p;
	parent->children.Add(this);
}

void CEditorMenu::Render()
{
	if (bHidden)
		return;

	if (name.IsEmpty())
	{
		RenderChildren();
		return;
	}

	if (children.Size() > 0)
	{
		if (ImGui::BeginMenu(name.c_str(), bEnabled))
		{
			RenderChildren();

			ImGui::EndMenu();
		}
	}
	else
	{
		if (ImGui::MenuItem(name.c_str(), shortcut.c_str(), bChecked, bEnabled))
		{
			if (bToggle)
			{
				bChecked^= 1;

				if (bChecked && OnEnabled)
					OnEnabled();
				else if (OnDisabled)
					OnDisabled();
			}

			if (OnClicked)
				OnClicked();
		}
	}
}

void CEditorMenu::RenderChildren()
{
	CEditorMenu* last = nullptr;
	for (auto* c : children)
	{
		if (c->bHidden)
			continue;

		if (((!last && !c->category.IsEmpty()) || (last && last->category != c->category)) && !name.IsEmpty())
		{
			bool bEmpty = c->category.IsEmpty();

			if (!bEmpty)
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 18));

			ImVec2 cursor = ImGui::GetCursorScreenPos();
			
			if (!bEmpty)
				ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y + 7));

			ImGui::Separator();

			if (!bEmpty)
				ImGui::PopStyleVar();

			ImColor background = ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg];

			ImVec2 textSize = ImGui::CalcTextSize(c->category.c_str());

			if (!bEmpty)
			{
				ImGui::RenderFrame(cursor, ImVec2(cursor.x + textSize.x + 5.f, cursor.y + 16), background);

				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.4f));
				ImGui::RenderText(ImVec2(cursor.x, cursor.y), c->category.c_str());
				ImGui::PopStyleColor();
			}
		}

		c->Render();
		last = c;
	}
}

void CEditorMenu::SortChildren()
{
	TMap<SizeType, TArray<CEditorMenu*>> sortedMenus;

	for (auto* c : children)
	{
		if (c->shortcut.IsEmpty())
			sortedMenus[-1].Add(c);
		else
			sortedMenus[c->shortcut.Hash()].Add(c);
	}

	children.Clear();

	for (auto& so : sortedMenus)
	{
		for (auto* c : so.second)
			children.Add(c);
	}
}
