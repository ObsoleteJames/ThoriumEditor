#pragma once

#include <Object/Object.h>
#include <Util/KeyValue.h>
#include "EditorCore.h"

class CLayer;
class CEditorMenu;

#define REGISTER_EDITOR_LAYER(Class, Menu, MenuCategory, AllowMultiple, EnableByDefault) \
class Class##_LayerType : public CLayerType \
{ \
public: \
	Class##_LayerType() \
	{ \
		className = #Class; \
		bAllowMultiple = AllowMultiple; \
		bEnabledByDefault = EnableByDefault; \
		if (Menu) \
			menuShortcut = Menu; \
		if (MenuCategory) \
			menuCategory = MenuCategory; \
		funcInstantiate = []() { return gEditorEngine()->AddLayer<Class>(); }; \
	} \
} Class##Type_Instance;

class SDK_API CLayerType
{
	friend class CLayer;

public:
	CLayerType();
	~CLayerType();

	CLayer* Instantiate();

	inline const FString& Name() const { return className; }
	inline const FString& Menu() const { return menuShortcut; }

	// wether or not multiple instances can exist
	inline bool AllowMultiple() const { return bAllowMultiple; }

	inline const TArray<CLayer*>& GetInstances() const { return instances; }
	inline CLayer* GetInstance() const { return instances.Size() > 0 ? instances[0] : nullptr;  }

protected:
	FString className;
	FString menuCategory;
	FString menuShortcut; // e.g.  "Tools/Material Editor"
	
	bool bAllowMultiple = false;
	bool bEnabledByDefault = false;

	std::function<CLayer*()> funcInstantiate;
	TArray<CLayer*> instances;
};

class SDK_API CLayer : public CObject
{	
	friend class CLayerType;

public:
	CLayer() = default;
	virtual ~CLayer();

	virtual void OnAttach() {}
	virtual void OnDetach() {}

	virtual void OnUpdate(double dt) {}
	virtual void OnUIRender() {}

	static const TArray<CLayerType*>& GetLayerTypes();
	static CLayerType* GetLayerType(const FString& name);

	static void Init();

	static void LoadConfig(FKeyValue& cfg);
	static void SaveConfig(FKeyValue& cfg);

	inline CEditorMenu* Menu() const { return menu; }

private:
	CLayerType* type = nullptr;
	CEditorMenu* menu = nullptr;

public:
	bool bEnabled = true;
};
