
#include "Widgets/ContentBrowser.h"
#include "Assets/ModelAsset.h"
#include "Misc/FileHelper.h"
#include "EditorEngine.h"
#include "ModelCompiler.h"

class FModelContextMenu : public FAssetBrowserAction
{
public:
	FModelContextMenu()
	{
		type = BA_FILE_CONTEXTMENU;
		targetClass = (FAssetClass*)CModelAsset::StaticClass();
	}

	void Invoke(FBrowserActionData* data) override
	{
		bool bCompilable = FFileHelper::FileExists(data->file->GetSdkPath(".meta"));

		FFile* f = data->file;

		data->menu->addAction("Edit Model...", data->browser, [=]() {
		});

		data->menu->addAction("Compile Model", data->browser, [=]() {
			CModelAsset* mdl = CAssetManager::GetAsset<CModelAsset>(f->Path());
			if (!mdl)
				return;

			mdl->Connect();

			gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
				CModelCompiler compiler;
				if (!compiler.CompileFromCfgFile(mdl, f->GetSdkPath(".meta"), false))
					CONSOLE_LogError("CModelCompiler", compiler.GetError());
				else
					mdl->Save();

				mdl->Disconnect();
			});
		})->setEnabled(bCompilable);

		data->menu->addAction("Re-Import", data->browser, [=]() {
			CModelAsset* mdl = CAssetManager::GetAsset<CModelAsset>(f->Path());
			if (!mdl)
				return;

			mdl->Connect();

			gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
				CModelCompiler compiler;
				if (!compiler.CompileFromCfgFile(mdl, f->GetSdkPath(".meta")))
					CONSOLE_LogError("CModelCompiler", compiler.GetError());
				else
					mdl->Save();

				mdl->Disconnect();
			});
		})->setEnabled(bCompilable);
	}
} static FModelContextMenu_instance;
