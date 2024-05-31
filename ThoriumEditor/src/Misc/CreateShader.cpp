
#include "AssetBrowserWidget.h"
#include "Rendering/Shader.h"

#include "ImGui/imgui.h"

#include <fstream>
#include <filesystem>

class FShaderCreateMenu : public FAssetBrowserAction
{
public:
	FShaderCreateMenu()
	{
		type = BA_WINDOW_CONTEXTMENU;
	}

	static void DoCreate(const FString& path, const FString& mod)
	{
		TObjectPtr<CShaderSource> shader = CResourceManager::CreateResource<CShaderSource>(path, mod);

		FString shaderName = ToFString(path);
		if (auto i = shaderName.FindLastOf("\\/"); i != -1)
			shaderName.Erase(shaderName.begin(), shaderName.begin() + i + 1);

		FString sdkDir = shader->File()->GetSdkPath();
		if (auto i = sdkDir.FindLastOf("\\/"); i != -1)
			sdkDir.Erase(sdkDir.begin() + i, sdkDir.end());

		std::filesystem::create_directories(sdkDir.c_str());

		std::ofstream sdkStream(shader->File()->GetSdkPath().c_str());
		if (!sdkStream.is_open())
			return;

		sdkStream << ("\nShader\n{\n\tName = \"" + shaderName + "\";\n\tType = SHADER_FORWARD;\n}\n\n").c_str()
			<< "Global\n{\n\t#include \"common/common.hlsl\"\n\n\tstruct VS_Input\n\t{\n\t\t#include \"common/vertex_input.hlsl\"\n\t};\n\n\tstruct PS_Input\n\t{\n\t\t#include \"common/pixel_input.hlsl\"\n\t};\n\n\tProperty<int> vTestProperty(Name = \"Test Property\");\n}\n\n"
			<< "VS\n{\n\t#include \"common/vertex.hlsl\"\n\n\tPS_Input Main(VS_Input input)\n\t{\n\t\tPS_Input output = ProcessVertex(input);\n\n\t\tFinalizeVertex(input, output);\n\t\treturn output;\n\t}\n}\n\n"
			<< "PS\n{\n\tfloat4 Main(PS_Input input) : SV_TARGET\n\t{\n\t\treturn float4(0.5f, 0.5f, 1.f, 1.f);\n\t}\n}\n";

		sdkStream.close();

		shader->Compile();
		shader->Init();
	}

	void Invoke(FBrowserActionData* data) override
	{
		if (ImGui::MenuItem("CreateShader"))
		{
			data->browser->PrepareNewFile(&FShaderCreateMenu::DoCreate, (FAssetClass*)CShaderSource::StaticClass());
		}
	}
} static FShaderCreateMenu_instance;
