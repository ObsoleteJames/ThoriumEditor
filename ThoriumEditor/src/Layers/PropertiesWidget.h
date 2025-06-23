#pragma once

#include "Layer.h"
#include "EngineCore.h"
#include "Math/Vectors.h"

class FStruct;
struct FProperty;
class CEntity;
class CEntityComponent;
class IPropertyEditor;

struct FPropertyCategory
{
	FString name;
	TArray<TUniquePtr<IPropertyEditor>> editors;
};

class CPropertiesWidget : public CLayer
{
public:
	void OnUIRender() override;

	void RenderClassProperties(FStruct* type, SizeType offset, bool bHeader = true);
	void RenderTransformEdit();

	bool RenderVectorProperty(SizeType offset, bool bReadOnly);
	bool RenderColorProperty(SizeType offset, bool bReadOnly);
	bool RenderQuatProperty(SizeType offset, bool bReadOnly);

	void RenderProperty(uint type, const FProperty* prop, void** objects, int objCount, SizeType offset);

	void AddComponent(FClass* type);

	void Refresh();

	FPropertyCategory* GetCategory(const FString& name);

	void AddProperties(FClass* type, int numObjects, CObject** objects, void** data);

public:
	CEntityComponent* selectedComp = nullptr;
	CEntity* prevEnt = nullptr;

	FVector rotCache;

	TArray<FPropertyCategory> categories;
};
