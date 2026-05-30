#pragma once

#include "Editor.h"

#include "Module.h"

struct EDITOR_API FEditorPlugin
{
	FString name;
	FString libPath;
	TArray<FString> dependancies;

	FLibrary* lib = nullptr;

	bool bEnabled;
	bool bInternal;
};

class EDITOR_API CEditorPlugins
{
public:
	static void Init();
	static void Exit();

	static FEditorPlugin* GetPlugin(const FString& name);
};
