
#include "Layer.h"
#include "EditorEngine.h"
#include "Resources/Scene.h"
#include "Game/World.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/ImGui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "EditorWidgets.h"

class CGameMode;

class CSceneSettingsWnd : public CLayer
{
public:
	CSceneSettingsWnd() = default;

	void OnUIRender() override;
};

REGISTER_EDITOR_LAYER(CSceneSettingsWnd, "View/Scene Settings", nullptr, false, true)

void CSceneSettingsWnd::OnUIRender()
{
	if (ImGui::Begin("Scene Settings"))
	{
		CScene* scene = gWorld ? gWorld->GetScene() : nullptr;
		
		if (ImGui::BeginTable("sceneSettingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
		{
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Value");
			ImGui::TableHeadersRow();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Gamemode");
			ImGui::TableNextColumn();

			TClassPtr<CGameMode>* gamemode = scene ? &scene->gamemodeClass : nullptr;
			ImGui::ClassPtrWidget<CGameMode>("##gamemodeSetting", &gamemode, 1);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Gravity");
			ImGui::TableNextColumn();
			float* gravity = scene ? &scene->gravity : nullptr;
			ImGui::DragFloat("##sceneGravity", gravity, 0.1f);

			ImGui::EndTable();
		}
	}
	ImGui::End();
}
