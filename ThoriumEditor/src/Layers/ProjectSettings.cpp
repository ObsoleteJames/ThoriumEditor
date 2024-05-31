
#include "ProjectSettings.h"
#include "EditorEngine.h"

#include "Game/GameInstance.h"
#include "Game/Input/InputManager.h"
#include "Resources/ResourceManager.h"
#include "Resources/Scene.h"

#include "Engine.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "EditorWidgets.h"
#include "EditorMenu.h"

REGISTER_EDITOR_LAYER(CProjectSettingsWidget, "Edit/Project Settings", "Settings", false, false)

struct FPSMenu
{
	const char* name;
	void(CProjectSettingsWidget::*renderFunc)();
};

static const FPSMenu menus[] = {
	{ "Game", &CProjectSettingsWidget::RenderGameSettings },
	{ "Audio", &CProjectSettingsWidget::RenderAudioSettings },
	{ "Input", &CProjectSettingsWidget::RenderInputSettings },
	{ "Physics", &CProjectSettingsWidget::RenderPhysicsSettings },
	{ "Rendering", &CProjectSettingsWidget::RenderRenderSettings }
};

static constexpr size_t menusCount = IM_ARRAYSIZE(menus);

void CProjectSettingsWidget::OnUIRender()
{
	if (ImGui::Begin("Project Settings", &bEnabled))
	{
		auto contentSize = ImGui::GetContentRegionAvail();

		if (ImGui::BeginChild("_psMenu", ImVec2(200, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding))
		{
			for (int i = 0; i < menusCount; i++)
			{
				if (ImGui::Selectable(menus[i].name, curMenu == i))
					curMenu = i;
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
		if (ImGui::BeginChild("_psSettings"))
		{
			ImGui::BeginDisabled(!gEngine->IsProjectLoaded());

			(this->*menus[curMenu].renderFunc)();

			ImGui::EndDisabled();
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();

		/*if (ImGui::BeginTable("_psSettingsTable", 2))
		{


			ImGui::EndTable();
		}*/
	}
	ImGui::End();
	Menu()->bChecked = bEnabled;
}

void CProjectSettingsWidget::RenderGameSettings()
{
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	ImGui::Text("Game");
	ImGui::PopFont();

	ImVec2 region = ImGui::GetContentRegionAvail();
	ImVec2 cursor = ImGui::GetCursorScreenPos();

	ImGui::RenderFrame(cursor, cursor + region, ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.1f, 0.1f, 1.f)), false);

	if (ImGui::BeginTable("_psSettingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Project Name");
		ImGui::TableNextColumn();
		if (ImGui::InputText("##_projectNameEdit", (FString*)&gEditorEngine()->GetProjectConfig().displayName, ImGuiInputTextFlags_EnterReturnsTrue))
			gEditorEngine()->SaveProjectConfig();

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Game Title");
		ImGui::TableNextColumn();
		if (ImGui::InputText("##_projectTitle", (FString*)&gEditorEngine()->ActiveGame().title, ImGuiInputTextFlags_EnterReturnsTrue))
			gEditorEngine()->SaveProjectConfig();

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Icon");
		ImGui::TableNextColumn();
		TObjectPtr<CTexture> icon;
		if (!gEditorEngine()->GetProjectConfig().iconPath.IsEmpty())
		{
			icon = CResourceManager::GetResource<CTexture>(gEditorEngine()->GetProjectConfig().iconPath);
			if (!icon)
				gEditorEngine()->GetProjectConfig().iconPath.Clear();
		}
		TObjectPtr<CTexture>* iconPtr = &icon;
		if (ImGui::AssetPtrWidget("##_projectIcon", (TObjectPtr<CAsset>**) & iconPtr, 1, (FAssetClass*)CTexture::StaticClass()))
		{
			if (icon)
				gEditorEngine()->GetProjectConfig().iconPath = icon->GetPath();
			else
				gEditorEngine()->GetProjectConfig().iconPath.Clear();

			gEditorEngine()->SaveProjectConfig();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Author");
		ImGui::TableNextColumn();
		if (ImGui::InputText("##_projectAuthorEdit", (FString*)&gEditorEngine()->GetProjectConfig().author, ImGuiInputTextFlags_EnterReturnsTrue))
			gEditorEngine()->SaveProjectConfig();

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Version");
		ImGui::TableNextColumn();
		if (ImGui::InputText("##_projectVersion", (FString*)&gEditorEngine()->ActiveGame().version, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank))
			gEditorEngine()->SaveProjectConfig();

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Game Instance Class");
		ImGui::TableNextColumn();
		TClassPtr<CObject>* gameInstanceEdit = (TClassPtr<CObject>*) & gEditorEngine()->ActiveGame().gameInstanceClass;
		if (ImGui::ClassPtrWidget("##_gameInstanceEdit", &gameInstanceEdit, 1, CGameInstance::StaticClass()))
			gEditorEngine()->SaveProjectConfig();
		
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Startup Scene");
		ImGui::TableNextColumn();
		TObjectPtr<CScene> scene;
		if (!gEditorEngine()->ActiveGame().startupScene.IsEmpty())
		{
			scene = CResourceManager::GetResource<CScene>(ToFString(gEditorEngine()->ActiveGame().startupScene));
			if (!scene)
				gEditorEngine()->ActiveGame().startupScene = FString();
		}
		TObjectPtr<CScene>* scenePtr = &scene;
		if (ImGui::AssetPtrWidget("##_gameStartupScene", (TObjectPtr<CAsset>**) & scenePtr, 1, (FAssetClass*)CScene::StaticClass()))
		{
			if (scene)
				gEditorEngine()->ActiveGame().startupScene = ToFString(scene->File()->Path());
			else
				gEditorEngine()->ActiveGame().startupScene = FString();

			gEditorEngine()->SaveProjectConfig();
		}

		ImGui::EndTable();
	}
}

void CProjectSettingsWidget::RenderAudioSettings()
{
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	ImGui::Text("Audio");
	ImGui::PopFont();
}

void CProjectSettingsWidget::RenderInputSettings()
{
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	ImGui::Text("Input");
	ImGui::PopFont();

	if (ImGui::BeginTable("tableInput", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
	{
		CInputManager* inputManager = gEngine->InputManager();

		if (ImGui::TableTreeHeader("Bindings"))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImVec2 content = ImGui::GetContentRegionAvail();
			
			if (ImGui::TreeNodeEx("Actions", ImGuiTreeNodeFlags_DefaultOpen))
			{
				int i = 0;
				for (auto& action : inputManager->GetActions())
				{
					i++;

					ImVec2 _c = ImGui::GetCursorScreenPos();

					if (ImGui::TreeNode(FString("##" + FString::ToString(i)).c_str()))
						ImGui::TreePop();

					ImGui::SetCursorScreenPos(_c + ImVec2(24, -5));
					ImGui::InputText(("##" + FString::ToString(i)).c_str(), &action.name);
				}

				ImGui::TreePop();
			}

			ImGui::SetCursorScreenPos(cursor + ImVec2(content.x - 29, - 5));
			if (ImGui::ButtonClear("Add##addAction"))
			{
				FInputAction action;
				action.name = "New Action";
				gEngine->InputManager()->GetActions().Add(action);
			}

			/*if (ImGui::ButtonClear("+##addAction", ImVec2(24, 24)))
			{
				FInputAction action;
				action.name = "New Action";
				gEngine->InputManager()->GetActions().Add(action);
			}*/

			ImGui::TreePop();
		}

		ImGui::EndTable();
	}
}

void CProjectSettingsWidget::RenderPhysicsSettings()
{
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	ImGui::Text("Physics");
	ImGui::PopFont();
}

constexpr const char* renderQualityTxt[] = {
	"Very Low",
	"Low",
	"Medium",
	"High",
	"Ultra",
};

void CProjectSettingsWidget::RenderRenderSettings()
{
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	ImGui::Text("Rendering");
	ImGui::PopFont();

	if (ImGui::BeginTable("tableInput", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Shadow Quality");
		ImGui::TableNextColumn();
		int t = cvRenderShadowQuality.AsInt();
		if (QualitySetting("##shadowQuality", &t, 0, 4))
			cvRenderShadowQuality.SetValue(t);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Texture Quality");
		ImGui::TableNextColumn();
		t = cvRenderTextureQuality.AsInt();
		if (QualitySetting("##texQuality", &t, 0, 4))
			cvRenderTextureQuality.SetValue(t);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Force Forward Rendering");
		ImGui::TableNextColumn();
		bool ffr = cvForceForwardRendering.AsBool();
		if (ImGui::Checkbox("##forceForwardRender", &ffr))
			cvForceForwardRendering.SetValue(ffr);

		ImGui::EndTable();
	}
}

bool CProjectSettingsWidget::QualitySetting(const char* label, int* value, int min, int max)
{
	int txtIndex = *value;
	if (max < 4)
		txtIndex++;

	bool r = false;

	if (ImGui::BeginCombo(label, renderQualityTxt[txtIndex]))
	{
		for (int i = min; i < max + 1; i++)
		{
			txtIndex = i;
			if (max < 4)
				txtIndex++;

			if (ImGui::Selectable(renderQualityTxt[txtIndex], *value == i))
			{
				*value = i;
				r = true;
			}
		}

		ImGui::EndCombo();
	}
	return r;
}
