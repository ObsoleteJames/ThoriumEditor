
#include "AssetBrowserWidget.h"
#include "Assets/Scene.h"
#include "Misc/FileHelper.h"
#include "EditorEngine.h"

#include "ImGui/imgui.h"

class FSceneOpenAction : public FAssetBrowserAction
{
public:
	FSceneOpenAction()
	{
		type = BA_OPENFILE;
		targetClass = (FAssetClass*)CScene::StaticClass();
	}

	void Invoke(FBrowserActionData* data) override
	{
		gEngine->LoadWorld(data->file->Path());
	}

} static FSceneOpenAction_Instance;
