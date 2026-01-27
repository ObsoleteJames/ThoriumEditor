
#include "Widgets/ContentBrowser.h"
#include "Rendering/Shader.h"
#include "Misc/FileHelper.h"
#include "EditorEngine.h"
#include "System.h"

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
		bool bCompilable = FFileHelper::FileExists(data->file->GetSdkPath(".hlsl"));

		FFile* f = data->file;

		QAction* action = data->menu->addAction("Compile Shader", data->browser, [=]() {
			class CompileShaderEvent : public IEditorEvent
			{
			public:
				CompileShaderEvent(CShaderSource* s) : shader(s) {}

				void Exec() override
				{
					shader->Compile();
				}

				TObjectPtr<CShaderSource> shader;
			};

			auto shader = CAssetManager::GetAsset<CShaderSource>(f->Path());
			auto* event = new CompileShaderEvent(shader);
			gEditorEngine->PushEvent(event);
		});
		action->setEnabled(bCompilable);

		action = data->menu->addAction("Edit Shader...", data->browser, [=]() {
			SSystem::OpenFile(f->GetSdkPath(".hlsl"));
		});
		action->setEnabled(bCompilable);

		/*ImGui::BeginDisabled(!FFileHelper::FileExists(data->file->GetSdkPath(".hlsl")));
		
		if (ImGui::MenuItem("Compile Shader"))
		{
			auto shader = CAssetManager::GetAsset<CShaderSource>(data->file->Path());
			shader->Compile();
		}

		if (ImGui::MenuItem("Edit Shader"))
		{
			SSystem::OpenFile(data->file->GetSdkPath(".hlsl"));
		}

		ImGui::EndDisabled();*/
	}

} static FShaderContextMenu_instance;
