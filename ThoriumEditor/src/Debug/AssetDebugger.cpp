
#include "AssetDebugger.h"
#include "EditorEngine.h"
#include "Assets/AssetManager.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Imgui/imgui_thorium.h"

REGISTER_EDITOR_LAYER(CAssetDebugger, "Debug/Asset Debugger", nullptr, false, false)

void CAssetDebugger::OnUIRender()
{
	if (ImGui::Begin("Asset Debugger", &bEnabled))
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
		if (ImGui::BeginChild("##assetList", ImVec2(200, 0)))
		{
			auto& assets = CAssetManager::GetAssetsData();

			bool bSelectedExists = false;
			for (auto& it : assets)
			{
				if (ImGui::Selectable(it.second.file->Name().c_str(), selected == it.first))
				{
					selected = it.first;
				}

				if (selected == it.first)
					bSelectedExists = true;
			}

			if (!bSelectedExists)
				selected = -1;
		}
		ImGui::PopStyleColor();
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
		if (ImGui::BeginChild("##assetViewer"))
		{
			if (ImGui::BeginTable("tableClassViewer", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
			{
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Value");
				ImGui::TableHeadersRow();

				if (selected != -1)
				{
					const FAssetData* data = CAssetManager::GetAssetData(selected);

					FString path = data->file->Path();

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Path");
					ImGui::TableNextColumn();
					ImGui::InputText("##nameEdit", &path, ImGuiInputTextFlags_ReadOnly);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("ID");
					ImGui::TableNextColumn();
					ImGui::Text(FString::ToString(data->id));

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Type");
					ImGui::TableNextColumn();
					ImGui::Text(data->type->GetInternalName());

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Asset Version");
					ImGui::TableNextColumn();
					ImGui::Text(FString::ToString(data->version));
				}
				ImGui::EndTable();
			}
		}
		ImGui::PopStyleColor();
		ImGui::EndChild();
	}
	ImGui::End();
}
