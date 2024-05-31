
#include "EditorEngine.h"
#include "AssetBrowserWidget.h"
#include "Resources/ModelAsset.h"
#include "Resources/Animation.h"
#include "Layers/ModelEditor.h"
#include "Layer.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_thorium.h"

enum EImportType
{
	IMPORT_MODEL,
	IMPORT_ANIMATION,
	IMPORT_COUNT
};

const char* importTypeTxt[] = {
	"Model",
	"Animation"
};

bool ColliderCombo(const char* label, EColliderImportType * v)
{
	const char* colliderTypes[] = {
		"Off",
		"Simple",
		"Complex"
	};

	bool r = false;

	if (ImGui::BeginCombo(label, colliderTypes[(int)*v]))
	{
		for (int i = 0; i < 3; i++)
		{
			if (ImGui::Selectable(colliderTypes[i], *v == i))
			{
				*v = (EColliderImportType)i;
				r = true;
			}
		}

		ImGui::EndCombo();
	}
	return r;
}

class FModelImportUI : public CLayer
{
public:
	FModelImportUI()
	{
		ImGui::OpenPopup("Import Model##modelImporterDialog");
	}

	void OnUIRender() override
	{
		ImGui::SetNextWindowSize(ImVec2(425, 615), ImGuiCond_FirstUseEver);
		
		// :D
		ImGui::OpenPopup("Import Model##modelImporterDialog");

		if (ImGui::BeginPopupModal("Import Model##modelImporterDialog"))
		{
			ImGui::TextDisabled(data.sourceFile.c_str());

			ImVec2 wndSize = ImGui::GetContentRegionAvail();

			if (ImGui::BeginTable("importModelTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImVec2(wndSize.x, wndSize.y - 32)))
			{
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Value");
				ImGui::TableHeadersRow();

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Import as");
				ImGui::TableNextColumn();
				if (ImGui::BeginCombo("##importAs", importTypeTxt[(int)importType]))
				{
					for (int i = 0; i < IMPORT_COUNT; i++)
					{
						bool bSelected = importType == i;
						if (ImGui::Selectable(importTypeTxt[i], bSelected))
							importType = (EImportType)i;
					}

					ImGui::EndCombo();
				}

				if (importType == IMPORT_MODEL)
				{
					if (ImGui::TableTreeHeader("Meshes", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Import Scale");
						ImGui::TableNextColumn();
						ImGui::DragFloat("##_importScale", &importScale, 0.1f);

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Auto setup LODs");
						ImGui::TableNextColumn();
						ImGui::Checkbox("##checkboxAutoLods", &bAutoLods);

						ImGui::BeginDisabled(!bAutoLods);
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("LOD suffix");
						ImGui::TableNextColumn();
						ImGui::InputText("##lodSuffix", &lodSuffix);
						ImGui::EndDisabled();

						ImGui::TreePop();
					}

					if (ImGui::TableTreeHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Create Materials");
						ImGui::TableNextColumn();
						ImGui::Checkbox("##createmats", &settings.bCreateMaterials);

						ImGui::BeginDisabled(!settings.bCreateMaterials);
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Output folder");
						ImGui::TableNextColumn();
						ImGui::InputText("##matsOut", &settings.materialsOut);

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Import Textures");
						ImGui::TableNextColumn();
						ImGui::Checkbox("##importextures", &settings.bImportTextures);

						if (ImGui::TableTreeHeader("Texture Import Settings", 0, true))
						{
							ImGui::BeginDisabled(!settings.bImportTextures);

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text("you're mom  :D");

							ImGui::EndDisabled(); // !settings.bImportTextures

							ImGui::TreePop();
						}
						ImGui::EndDisabled(); // !settings.bCreateMaterials

						ImGui::TreePop();
					}

					if (ImGui::TableTreeHeader("Animations", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Import Animations");
						ImGui::TableNextColumn();
						ImGui::Checkbox("##importAnims", &bImportAnimations);

						ImGui::BeginDisabled(!bImportAnimations);

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Output folder");
						ImGui::TableNextColumn();
						ImGui::InputText("##animsOut", &animSettings.outputFolder);

						ImGui::EndDisabled(); // !bImportAnimations

						ImGui::TreePop();
					}

					if (ImGui::TableTreeHeader("Collision", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Use meshes as collision");
						ImGui::TableNextColumn();
						//ImGui::Checkbox("##meshascollider", &settings.bUseMeshesAsCollision);
						if (ColliderCombo("##meshCollider", &settings.bUseMeshesAsCollision))
						{
							if (bUseBoundsAsCollision == settings.bUseMeshesAsCollision)
								bUseBoundsAsCollision = COLLIDER_OFF;
							if (bGenerateConvexCollision == settings.bUseMeshesAsCollision)
								bGenerateConvexCollision = COLLIDER_OFF;
						}

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Use bounds as collision");
						ImGui::TableNextColumn();
						if (ColliderCombo("##boundsascollider", &bUseBoundsAsCollision))
						{
							if (settings.bUseMeshesAsCollision == bUseBoundsAsCollision)
								settings.bUseMeshesAsCollision = COLLIDER_OFF;
							if (bGenerateConvexCollision == bUseBoundsAsCollision)
								bGenerateConvexCollision = COLLIDER_OFF;
						}

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Generate convex collision");
						ImGui::TableNextColumn();
						if (ColliderCombo("##genconvex", &bGenerateConvexCollision))
						{
							if (settings.bUseMeshesAsCollision == bGenerateConvexCollision)
								settings.bUseMeshesAsCollision = COLLIDER_OFF;
							if (bUseBoundsAsCollision == bGenerateConvexCollision)
								bUseBoundsAsCollision = COLLIDER_OFF;
						}

						ImGui::TreePop();
					}
				}
				else if (importType == IMPORT_ANIMATION)
				{
					if (ImGui::TableTreeHeader("Animations", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Output folder");
						ImGui::TableNextColumn();
						ImGui::InputText("##animsOut", &animSettings.outputFolder);

						ImGui::TreePop();
					}
				}

				ImGui::EndTable();
			}

			if (ImGui::Button("Import"))
			{
				TObjectPtr<CModelAsset> mdl = CreateObject<CModelAsset>();
				THORIUM_ASSERT(CResourceManager::RegisterNewResource(mdl, data.outPath, data.outMod), "Failed to register CModelAsset!");

				FMeshFile mesh;
				mesh.file = data.sourceFile;
				mesh.transform.scale = FVector(importScale);

				CModelCompiler compiler;
				compiler.Compile(mdl, &mesh, 1, settings);

				if (bAutoLods)
					compiler.GenerateLODGroups(lodSuffix);

				if (bGenerateConvexCollision)
					compiler.GenerateConvexCollision();

				compiler.SaveModel(&mesh, 1);

				ImGui::CloseCurrentPopup();
				gEditorEngine()->PollRemoveLayer(this);
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
				gEditorEngine()->PollRemoveLayer(this);
			}

			ImGui::EndPopup();
		}
	}

public:
	EImportType importType = IMPORT_MODEL;

	bool bAutoLods = true;
	FString lodSuffix = "_LOD";

	float importScale = 1.f;

	EColliderImportType bUseBoundsAsCollision = COLLIDER_OFF;
	EColliderImportType bGenerateConvexCollision = COLLIDER_SIMPLE;

	bool bImportAnimations = true;

	FAnimationImportSettings animSettings;
	FModelCompileSettings settings;

	FBAImportFile data;
};

class FModelImportAction : public FAssetBrowserAction
{
public:
	FModelImportAction()
	{
		type = BA_FILE_IMPORT;
		targetClass = (FAssetClass*)CModelAsset::StaticClass();
	}

	void Invoke(FBrowserActionData* d) override
	{
		FBAImportFile* data = (FBAImportFile*)d;

		FModelImportUI* ui = gEditorEngine()->AddLayer<FModelImportUI>();
		ui->data = *data;
		ui->bEnabled = true;
	}
} static ModelImportActionInstance;
