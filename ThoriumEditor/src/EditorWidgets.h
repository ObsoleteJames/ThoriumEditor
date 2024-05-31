#pragma once

#include "EditorCore.h"
#include "Object/Object.h"

#include "ImGui/imgui.h"

class CAsset;

namespace ImGui
{
	SDK_API bool ObjectPtrWidget(const char* label, TObjectPtr<CObject>** values, int numValues, FClass * filterClass);
	SDK_API bool AssetPtrWidget(const char* label, TObjectPtr<CAsset>** values, int numValues, FAssetClass* filterClass);
	SDK_API bool ClassPtrWidget(const char* label, TClassPtr<CObject>** values, int numValues, FClass* filterClass);

	template<typename T>
	SDK_API bool ObjectPtrWidget(const char* label, TObjectPtr<T>** values, int numValues) { return ObjectPtrWidget(label, (TObjectPtr<CObject>**)values, numValues, T::StaticClass()); }

	template<typename T>
	SDK_API bool AssetPtrWidget(const char* label, TObjectPtr<T>** values, int numValues) { return AssetPtrWidget(label, (TObjectPtr<CAsset>**)values, numValues, (FAssetClass*)T::StaticClass()); }

	template<typename T>
	SDK_API bool ClassPtrWidget(const char* label, TClassPtr<T>** values, int numValues) { return ClassPtrWidget(label, (TClassPtr<CObject>**)values, numValues, T::StaticClass()); }
}
