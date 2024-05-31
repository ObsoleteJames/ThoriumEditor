
#include "AssetBrowserWidget.h"
#include "Rendering/Shader.h"

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
		if (ImGui::MenuItem("Compile"))
		{
			auto shader = CResourceManager::GetResource<CShaderSource>(data->file->Path());
			shader->Compile();
		}
	}

} static FShaderContextMenu_instance;
