
#include "AddonsWindow.h"
#include "Engine.h"
#include "EditorEngine.h"
#include "EditorMenu.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/ImGui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "EditorWidgets.h"

REGISTER_EDITOR_LAYER(CAddonsWindow, "Edit/Addons", "Settings", false, false)

CAddonsWindow::CAddonsWindow()
{

}

void CAddonsWindow::OnUpdate(double dt)
{

}

void CAddonsWindow::OnUIRender()
{
	if (ImGui::Begin("Addons", &bEnabled))
	{
		//ImVec2 size = ImGui::GetContentRegionAvail();
		//sizeR = size.x - sizeL - 5;

		//ImGui::Splitter("##assetBorwserSplitter", false, 4.f, &sizeL, &sizeR, 32.f);

		if (!editingAddon)
		{
			if (ImGui::BeginTable("addonsTable", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
			{
				if (ImGui::TableTreeHeader("Core Addons", ImGuiTreeNodeFlags_DefaultOpen))
				{
					for (auto& addon : gEngine->GetAddons())
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();

						ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
						ImGui::Text(addon.name.c_str());
						ImGui::PopFont();

						ImGui::TextWrapped(addon.description.c_str());

						ImGui::Spacing();

						bool bEnabled = false;
						for (auto& a : gEngine->GetProjectConfig().addons)
						{
							if (a == "core:" + addon.identity)
							{
								bEnabled = true;
								break;
							}
						}

						ImVec2 pos = ImGui::GetCursorScreenPos();
						if (ImGui::Checkbox(("Enabled##_" + addon.identity).c_str(), &bEnabled))
						{
							TArray<FString>& addons = *(TArray<FString>*)&gEngine->GetProjectConfig().addons;
							
							if (!bEnabled)
							{
								auto it = addons.Find("core:" + addon.identity);
								if (it != addons.end())
									addons.Erase(it);
							}
							else
							{
								addons.Add("core:" + addon.identity);
							}

							gEditorEngine()->SaveProjectConfig();
						}

						ImGui::SetCursorScreenPos(pos + ImVec2(ImGui::GetContentRegionAvail().x - 30, 0));

						ImGui::Button(("Edit##" + addon.identity).c_str());
					}
					ImGui::TreePop();
				}

				if (ImGui::TableTreeHeader("Game Addons", ImGuiTreeNodeFlags_DefaultOpen))
				{
					for (auto& addon : gEngine->GetProjectConfig().projectAddons)
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();

						ImGui::Text(addon.name.c_str());
						ImGui::TextWrapped(addon.description.c_str());

						ImGui::Spacing();

						bool bEnabled = false;
						for (auto& a : gEngine->GetProjectConfig().addons)
						{
							if (a == addon.identity)
							{
								bEnabled = true;
								break;
							}
						}

						ImVec2 pos = ImGui::GetCursorScreenPos();
						if (ImGui::Checkbox(("Enabled##_" + addon.identity).c_str(), &bEnabled))
						{
							TArray<FString>& addons = *(TArray<FString>*) & gEngine->GetProjectConfig().addons;

							if (!bEnabled)
							{
								auto it = addons.Find(addon.identity);
								if (it != addons.end())
									addons.Erase(it);
							}
							else
							{
								addons.Add(addon.identity);
							}
						}

						ImGui::SetCursorScreenPos(pos + ImVec2(ImGui::GetContentRegionAvail().x - 30, 0));

						ImGui::Button(("Edit##" + addon.identity).c_str());
					}
					ImGui::TreePop();
				}

				ImGui::EndTable();
			}
		}

		//if (ImGui::BeginChild("addonsList"))
		//{
			//if (ImGui::TreeNode("Core Addons"))
			//{
			//	for (auto& addon : gEngine->GetAddons())
			//	{
			//		if (ImGui::Selectable(addon.name.c_str(), &addon == editingAddon))
			//			editingAddon = &addon;
			//	}

			//	ImGui::TreePop();
			//}

			//if (ImGui::TreeNode((gEngine->ActiveGame().name + " Addons").c_str()))
			//{
			//	for (auto& addon : gEngine->GetProjectConfig().projectAddons)
			//	{
			//		if (ImGui::Selectable(addon.name.c_str(), &addon == editingAddon))
			//			editingAddon = &addon;
			//	}

			//	ImGui::TreePop();
			//}
		//}
		//ImGui::EndChild();

		//ImGui::SameLine();

		//if (ImGui::BeginChild("addonsRight", ImVec2(sizeR, 0)))
		//{

		//}
		//ImGui::EndChild();
	}
	ImGui::End();
	Menu()->bChecked = bEnabled;
}

void CAddonsWindow::DrawAddonList()
{

}

void CAddonsWindow::DrawAddonEditor()
{

}
