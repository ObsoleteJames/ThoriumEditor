#pragma once

#include "EngineCore.h"

struct FFile;
struct FMod;
struct FDirectory;
class FAssetClass;
class CAssetBrowserWidget;

enum EBrowserActionType
{
	BA_INVALID,
	BA_OPENFILE, // file has been double clicked
	BA_FILE_CONTEXTMENU, // draw context menu for file
	BA_FILE_IMPORT,
	// BA_FILE_REIMPORT - Use context menu for this
	BA_FILE_DUPLICATE,
	BA_WINDOW_CONTEXTMENU
};

struct FBADataBase
{
	CAssetBrowserWidget* browser;
	FFile* file;
};
struct FBADataDuplicate : public FBADataBase 
{
	FFile* sourceFile;
};
struct FBAImportFile : public FBADataBase
{
	FString sourceFile;
	FString outPath;
	FString outMod;
};
struct FBAWindowContext : public FBADataBase
{
	FString mod;
	FString dir;
};

typedef FBADataBase FBrowserActionData;

class FAssetBrowserAction
{
	typedef TArray<FAssetBrowserAction*> FActionList;

public:
	FAssetBrowserAction();

	virtual void Invoke(FBrowserActionData* data) = 0;

	inline EBrowserActionType Type() const { return type; }
	inline FAssetClass* TargetClass() const { return targetClass; }

	inline static const FActionList& GetActions() { return actions; }

protected:
	EBrowserActionType type;
	FAssetClass* targetClass;

	static FActionList actions;
};

class CAssetBrowserWidget
{
public:
	// Must be called inside of a window or child window.
	void RenderUI(float width = 0.f, float height = 0.f);

	inline FFile* GetSelectedFile() const { if (selectedFiles.Size() == 0) return nullptr; return selectedFiles[0]; }
	inline const TArray<FFile*>& GetSelectedFiles() const { return selectedFiles; }

	void SetDir(FMod* m, FDirectory* d);
	void SetDir(const FString& m, const FString& d);

	// Operators
	void Back();
	void Forward();
	void Root();

	// Extract the mod and dir, format: 'MOD:\DIR\DIR2'
	bool ExtractPath(const FString& combined, FString& outMod, FString& outDir);

	void PrepareNewFile(void(*onFinishFun)(const FString& outPath, const FString& mod) = nullptr, FAssetClass* type = nullptr);
	void PrepareNewFolder();

private:
	void DrawDirTree(FDirectory* dir, FDirectory* parent, FMod* mod);

	void SetSelectedFile(FFile* file);
	void AddSelectedFile(FFile* file);
	void RemoveSelectedFile(FFile* file);
	bool IsFileSelected(FFile* file);

	void ImportAsset();

public:
	bool bAllowFileEdit = true;
	bool bAllowMultiSelect = true;

	// true if the selected file was double clicked
	bool bDoubleClickedFile = false;

	// if set, only assets of this type will be visible
	FAssetClass* viewFilter = nullptr;

	FString dir;
	FString mod = "Engine";

	int iconsSize = 2;

	FString newFileStr;
	FAssetClass* newFileType = nullptr;
	bool bCreatingFile = false;
	bool bCreatingFolder = false;
	void(*onCreatedFileFun)(const FString& outPath, const FString& mod) = nullptr;

private:
	FString dirInput;

	SizeType historyIndex = 0;
	TArray<FString> historyList;

	TArray<FFile*> selectedFiles;

	float sizeL = 200;
	float sizeR = 200;

};
