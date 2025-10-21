
#include "Widgets/ContentBrowser.h"
#include "Assets/Scene.h"
#include "Misc/FileHelper.h"
#include "EditorEngine.h"

class FSceneOpenAction : public FAssetBrowserAction
{
public:
	FSceneOpenAction()
	{
		type = BA_FILE_OPEN;
		targetClass = (FAssetClass*)CScene::StaticClass();
	}

	void Invoke(FBrowserActionData* data) override
	{
		gEngine->LoadWorld(data->file->Path());
	}

} static FSceneOpenAction_Instance;
