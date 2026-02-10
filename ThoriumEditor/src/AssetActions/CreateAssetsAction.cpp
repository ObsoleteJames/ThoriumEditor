
#include "Widgets/ContentBrowser.h"
#include "Assets/Material.h"
#include "Assets/ModelAsset.h"
#include "Assets/Animation.h"

class FCreateMaterialAction : public FAssetBrowserAction
{
public:
	FCreateMaterialAction()
	{
		type = BA_WINDOW_CONTEXTMENU;
	}

	static void Make(const FString& path, const FString& mod)
	{
		TObjectPtr<CMaterial> mat = CAssetManager::CreateAsset<CMaterial>(path, mod);
		mat->SetShader("Simple");
		mat->Save();
	}

	void Invoke(FBrowserActionData* data)
	{
		//if (ImGui::MenuItem("Create Material"))
		//	data->browser->PrepareNewFile(&Make, (FAssetClass*)CMaterial::StaticClass());
		data->menu->addAction("Create Material", [=]() {
			data->browser->PrepareNewFile((FAssetClass*)CMaterial::StaticClass(), &Make);
		});
	}
} static FCreateMaterialAction_Instance;

class FCreateModelAction : public FAssetBrowserAction
{
public:
	FCreateModelAction()
	{
		type = BA_WINDOW_CONTEXTMENU;
	}

	static void Make(const FString& path, const FString& mod)
	{
		TObjectPtr<CModelAsset> mdl = CAssetManager::CreateAsset<CModelAsset>(path, mod);
		mdl->Save();
	}

	void Invoke(FBrowserActionData* data)
	{
		//if (ImGui::MenuItem("Create Model"))
		//	data->browser->PrepareNewFile(&Make, (FAssetClass*)CModelAsset::StaticClass());
		data->menu->addAction("Create Model", [=]() {
			data->browser->PrepareNewFile((FAssetClass*)CModelAsset::StaticClass(), &Make);
		});
	}
} static FCreateModelAction_Instance;

class FCreateAnimAction : public FAssetBrowserAction
{
public:
	FCreateAnimAction()
	{
		type = BA_WINDOW_CONTEXTMENU;
	}

	static void Make(const FString& path, const FString& mod)
	{
		/*TObjectPtr<CModelAsset> mdl = CAssetManager::CreateAsset<CModelAsset>(path, mod);
		mdl->Save();*/
	}

	void Invoke(FBrowserActionData* data)
	{
		//if (ImGui::MenuItem("Create Animation"))
		//	;//data->browser->PrepareNewFile(&Make, (FAssetClass*)CAnimation::StaticClass());
		data->menu->addAction("Create Animation", [=]() {
			data->browser->PrepareNewFile((FAssetClass*)CAnimation::StaticClass(), &Make);
		});
	}
} static FCreateAnimAction_Instance;

