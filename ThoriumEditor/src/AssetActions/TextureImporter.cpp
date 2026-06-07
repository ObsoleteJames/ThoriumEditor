
#include "Widgets/ContentBrowser.h"
#include "Assets/TextureAsset.h"
#include "Console.h"

class FTextureImportAction : public FAssetImportAction
{
public:
	FTextureImportAction()
	{
		targetClass = (FAssetClass*)CTexture::StaticClass();
		importableTypes = ".png;.jpg;.tga";
	}

	void Invoke(FBrowserActionData* d) override
	{
		FBAImportFile* data = (FBAImportFile*)d;

		/*CTexture* tex = CreateObject<CTexture>();
		THORIUM_ASSERT(CAssetManager::RegisterNewAsset(tex, data->outPath, data->outMod), "Failed to register CTexture asset!");

		if (!tex->Import(ToFString(data->sourceFile)))
		{
			CONSOLE_LogError("CTexture", "Failed to import Texture asset!");
			data->file->Mod()->DeleteFile(data->outPath);
		}*/
	}
} static FTextureImportAction_instance;
