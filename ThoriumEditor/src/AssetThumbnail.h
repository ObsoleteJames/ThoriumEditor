#pragma once

#include "Editor.h"
#include <QPixmap>

class CAsset;
class FClass;
class IAssetThumbnailGenerator;
class CEditorVar;

extern EDITOR_API CEditorVar evThumbnailMaxSize;

enum class EThumbnailState
{
	Unloaded,
	Loading,
	Loaded
};

struct EDITOR_API FAssetThumbnail
{
	QPixmap image;
	SizeType assetId = -1;
	EThumbnailState state = EThumbnailState::Unloaded;
	SizeType lastAccessed = 0;
};

class EDITOR_API FAssetThumbnailProvider
{
public:
	FAssetThumbnailProvider(const FString& name, FClass* targetType, std::function<IAssetThumbnailGenerator* (CAsset*, FAssetThumbnail*)>&& func);

	inline IAssetThumbnailGenerator* CreateGenerator(CAsset* target, FAssetThumbnail* th) const {
		return funCreateGenerator(target, th);
	}

	inline const FString& GetName() const { return name; }
	inline FClass* GetTargetType() const { return targetType; }

protected:
	std::function<IAssetThumbnailGenerator*(CAsset*, FAssetThumbnail*)> funCreateGenerator;

	FString name;
	FClass* targetType;
};

class EDITOR_API IAssetThumbnailGenerator
{
public:
	IAssetThumbnailGenerator() = default;
	
	inline bool IsDone() const { return bIsDone; }

	virtual bool IsReady() = 0;
	virtual void Execute() = 0; // execute the thumbnail generation.

protected:
	bool bIsDone = false; // is this done generating the thumbnail.
};

class EDITOR_API CAssetThumbnailManager
{
public:
	static FAssetThumbnail* GetThumbnail(CAsset* asset);
	static void ResetThumbnail(CAsset* asset);

	static void ClearThumbnails();

	// check wether any new thumbnails have recently been generated.
	static bool NewThumbnailsAvailable();

	static void Update();
};
