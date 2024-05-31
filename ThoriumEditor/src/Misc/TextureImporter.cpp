
#include "AssetBrowserWidget.h"
#include "Resources/TextureAsset.h"
#include "Console.h"

class FTextureImportAction : public FAssetBrowserAction
{
public:
	FTextureImportAction()
	{
		type = BA_FILE_IMPORT;
		targetClass = (FAssetClass*)CTexture::StaticClass();
	}

	void Invoke(FBrowserActionData* d) override
	{
		FBAImportFile* data = (FBAImportFile*)d;

		CTexture* tex = CreateObject<CTexture>();
		THORIUM_ASSERT(CResourceManager::RegisterNewResource(tex, data->outPath, data->outMod), "Failed to register CTexture asset!");

		if (!tex->Import(ToFString(data->sourceFile)))
		{
			CONSOLE_LogError("CTexture", "Failed to import Texture asset!");
			data->file->Mod()->DeleteFile(data->outPath);
		}
	}
} static FTextureImportAction_instance;
