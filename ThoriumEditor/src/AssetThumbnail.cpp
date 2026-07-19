
#include "AssetThumbnail.h"
#include "EditorConfig.h"
#include "Assets/Asset.h"

#include <mutex>

EDITOR_API CEditorVar evThumbnailMaxSize("Asset Thumbnail Max Size", "Appearance", FVariant(128), true);
EDITOR_API CEditorVar evMaxThumbnails("Maximum Loaded Asset Thumbnails", "Appearance", FVariant(100), true);

static TMap<SizeType, FAssetThumbnail*> thumbnails;
static TArray<IAssetThumbnailGenerator*> thumbnailGenerators;

static std::mutex mutexGenerators;

static bool bNewAvail = false;

TMap<FClass*, FAssetThumbnailProvider*>& GetThumbnailProviders()
{
	static TMap<FClass*, FAssetThumbnailProvider*> thumbnailProviders;
	return thumbnailProviders;
}

FAssetThumbnailProvider* GetThumbnailProvider(FClass* t)
{
	if (auto it = GetThumbnailProviders().find(t); it != GetThumbnailProviders().end())
		return it->second;

	return nullptr;
}

FAssetThumbnailProvider::FAssetThumbnailProvider(const FString& n, FClass* t, std::function<IAssetThumbnailGenerator* (CAsset*, FAssetThumbnail*)>&& func)
	: name(n), targetType(t), funCreateGenerator(func)
{
	GetThumbnailProviders()[t] = this;
}

void CheckForMaxThumbnails()
{
	while (thumbnails.size() > evMaxThumbnails.GetValue().AsInt())
	{
		FAssetThumbnail* oldest = nullptr;
		for (auto& t : thumbnails)
		{
			if (!oldest || oldest->lastAccessed > t.second->lastAccessed)
				oldest = t.second;
		}

		if (oldest)
		{
			thumbnails.erase(oldest->assetId);
			delete oldest;
		}
	}
}

FAssetThumbnail* CAssetThumbnailManager::GetThumbnail(CAsset* asset)
{
	SizeType assetId = asset->AssetId();
	auto it = thumbnails.find(assetId);
	if (it != thumbnails.end())
	{
		it->second->lastAccessed = (SizeType)std::time(nullptr);
		return it->second;
	}

	FAssetThumbnailProvider* provider = GetThumbnailProvider(asset->GetClass());
	if (!provider)
		return nullptr;

	auto* thumbnail = new FAssetThumbnail();
	thumbnail->assetId = assetId;
	thumbnail->lastAccessed = (SizeType)std::time(nullptr);
	thumbnails[assetId] = thumbnail;

	mutexGenerators.lock();
	thumbnailGenerators.Add(provider->CreateGenerator(asset, thumbnail));
	mutexGenerators.unlock();

	CheckForMaxThumbnails();
	return thumbnail;
}

void CAssetThumbnailManager::ResetThumbnail(CAsset* asset)
{
}

void CAssetThumbnailManager::ClearThumbnails()
{
}

bool CAssetThumbnailManager::NewThumbnailsAvailable()
{
	return bNewAvail;
}

void CAssetThumbnailManager::Update()
{
	bNewAvail = false;
	mutexGenerators.lock();
	auto gens = thumbnailGenerators;
	for (auto* gen : gens)
	{
		bool bDone = false;
		if (gen->IsReady() && !gen->IsDone())
		{
			gen->Execute();
			bDone = true;
			bNewAvail = true;
		}

		if (gen->IsDone())
		{
			thumbnailGenerators.Erase(thumbnailGenerators.Find(gen));
			delete gen;
		}

		// only generate one thumbnail per frame, as to not stall the engine.
		if (bDone)
			break;
	}
	mutexGenerators.unlock();
}
