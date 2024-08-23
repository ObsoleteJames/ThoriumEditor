#pragma once

#include "EngineCore.h"
#include "EditorCore.h"

class FLibrary;
class FEditorModule;

struct SDK_API FEditorAddon
{
public:
	FString name;
	FString identity;
	SizeType uid;

	FLibrary* library = nullptr;
	FEditorModule* module = nullptr;
	bool bEnabled = false;
};

class SDK_API FEditorModule
{
	friend class CEditorEngine;

public:
	FEditorModule(const char* name);

	inline const FString& Name() const { return name; }
	inline const TArray<FLibrary*>& GetLibraries() const { return libs; }

public:
	virtual void Init() {}
	virtual void Shutdown() {}

	virtual void Update() {}

private:
	FString name;

	// loaded dependancies
	TArray<FLibrary*> libs;
};

#define REGISTER_EDITOR_ADDON(mod) \
extern "C" __declspec(dllexport) FEditorModule* GetAddonModule() \
{ \
	return &mod; \
}

#define DECLARE_GENERIC_EDITOR_ADDON(name) \
static FEditorModule  name##_module(#name); \
REGISTER_EDITOR_ADDON(##name##_module)
