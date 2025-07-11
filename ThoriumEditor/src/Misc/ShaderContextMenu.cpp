
#include "AssetBrowserWidget.h"
#include "Rendering/Shader.h"
#include "Misc/FileHelper.h"
#include "EditorEngine.h"

#include "ImGui/imgui.h"

class FShaderContextMenu : public FAssetBrowserAction
{
public:
	FShaderContextMenu()
	{
		type = BA_FILE_CONTEXTMENU;
		targetClass = (FAssetClass*)CShaderSource::StaticClass();
	}

	void Invoke(FBrowserActionData* data) override
	{
		ImGui::BeginDisabled(!FFileHelper::FileExists(data->file->GetSdkPath(".hlsl")));
		
		if (ImGui::MenuItem("Compile Shader"))
		{
			auto shader = CAssetManager::GetAsset<CShaderSource>(data->file->Path());
			shader->Compile();
		}

		if (ImGui::MenuItem("Edit Shader"))
		{
			CEditorEngine::OSOpenFile(data->file->GetSdkPath(".hlsl"));
		}

		ImGui::EndDisabled();
	}

} static FShaderContextMenu_instance;
