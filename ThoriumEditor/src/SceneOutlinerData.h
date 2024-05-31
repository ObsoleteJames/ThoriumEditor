#pragma once

#include "EngineCore.h"
#include "EditorCore.h"
#include "Math/Color.h"

#include <Util/Map.h>

struct FSceneOutlinerFolder
{
	FString name;
	FColor color;
	FGuid id;

	TArray<FSceneOutlinerFolder> childFolders;
};

struct FSceneOutlinerTree
{
	TArray<FSceneOutlinerFolder> folders;

	// Key: EntityID, Value: { FolderId, SortOrder }
	TMap<SizeType, TPair<SizeType, int>> outlinerOrder;
};
