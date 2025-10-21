#pragma once

#include <QDockWidget>
#include <QMenu>
#include <Util/String.h>
#include "Editor.h"

struct FDirectory;
struct FFile;
struct FMod;
class FAssetClass;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QSplitter;
class QSlider;

class CContentBrowserWidget;

class CAssetFilterMenu : public QMenu
{
	Q_OBJECT

public:
	CAssetFilterMenu(TArray<FString>* filterList, QWidget* parent = nullptr);
	~CAssetFilterMenu();

	void SetFilter(const FString& ext, bool enabled);

Q_SIGNALS:
	void OnFilterUpdate();

private:
	TArray<FString>* filterList;
	TArray<QAction*> actions;

};

enum EBrowserActionType
{
	BA_INVALID,
	BA_FILE_OPEN, // file has been double clicked
	BA_FILE_CONTEXTMENU, // draw context menu for file
	BA_FILE_IMPORT,
	// BA_FILE_REIMPORT - Use context menu for this
	BA_FILE_DUPLICATE,
	BA_WINDOW_CONTEXTMENU
};

struct FBADataBase
{
	CContentBrowserWidget* browser;
	QMenu* menu;
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

class EDITOR_API FAssetBrowserAction
{
	typedef TArray<FAssetBrowserAction*> FActionList;

public:
	FAssetBrowserAction();

	virtual void Invoke(FBrowserActionData* data) = 0;

	inline EBrowserActionType Type() const { return type; }
	inline FAssetClass* TargetClass() const { return targetClass; }

	inline static const FActionList& GetActions() { return _Actions(); }

	static FAssetBrowserAction* GetAction(FAssetClass* target, EBrowserActionType type = BA_INVALID);

private:
	static FActionList& _Actions();

protected:
	EBrowserActionType type;
	FAssetClass* targetClass;
};

class EDITOR_API CContentBrowserWidget : public QWidget
{
	Q_OBJECT

public:
	enum EViewMode
	{
		VIEW_GRID,
		VIEW_LIST
	};

public:
	CContentBrowserWidget(QWidget* parent = nullptr);
	virtual ~CContentBrowserWidget();

	/*
	 *	Sets the current directory.
	 * 
	 *	Format: "Mod:\Dir\Dir2\etc..."
	 */
	void SetDirectory(const FString& dir, bool fromHistory = false);
	void SetDirectory(const FString& mod, const FString& dir, bool fromHistory = false);
	inline FString GetDirectory() const { return dir; }
	inline FString GetMod() const { return mod; }

	void SetGridSize(int size);
	inline int GridSize() const { return dirViewSize; }

	void AllowFileCreation() { bCreateFiles = true; }
	void DisableFileCreation() { bCreateFiles = false; }

	void LockAssetFilter();
	inline void AddAssetFilter(FString fileExt) { activeFilters.Add(fileExt); UpdateView(); }

	inline EViewMode ViewMode() const { return bDirViewGrid ? VIEW_GRID : VIEW_LIST; }
	inline void SetViewMode(EViewMode vm) { bDirViewGrid = vm == VIEW_GRID; }

	inline FFile* SelectedFile() const { return selectedFile; }
	inline const TArray<FFile*>& SelectedFiles() const { return selectedFiles; }

	inline QSplitter* GetSplitter() const { return splitter; }

	//static void RegisterAssetCreateMenu(const FAssetCreateMenu& cm) { assetMenus.Add(cm); }
	//static void RegisterAssetCreateMenu(const QString& name, void(*func)(const FString& path)) { assetMenus.Add({ name, func }); }

	inline void Refresh() { UpdateView(); }

private:
	void OnAssetUpdate();

	void UpdateView();
	void UpdateViewSettings();

	FDirectory* GetFDirectory(const FString& path);
	FDirectory* GetFDirectory(const FString& mod, const FString& path);

	void AddDirToTree(FMod* mod, FDirectory* dir, QTreeWidgetItem* parent);
	void CreateContextMenu(QPoint point);

	void ImportAsset();

	bool ExtractPath(const FString& combined, FString& outMod, FString& outDir);

	//static TArray<FAssetCreateMenu> assetMenus;

Q_SIGNALS:
	void fileDoubleClicked();
	void fileClicked();

private:
	int dirViewSize = 3;
	bool bCreateFiles = true;
	bool bDirViewGrid = true;
	bool bAllowMultiSelect = true;
	bool bFiltersLocked = false;

	TArray<FString> activeFilters;

	SizeType historyIndex = 0;
	TArray<FString> dirHistory;
	//FString curDir;

	FString mod;
	FString dir;

	CAssetFilterMenu* filterMenu;
	QTreeWidget* fileTree;
	QListWidget* dirView;

	QLineEdit* curFolderEdit;

	FFile* selectedFile;
	TArray<FFile*> selectedFiles;

	QSlider* gridSizeSlider;
	QPushButton* btnViewMode;
	QPushButton* btnRootFolder;
	QPushButton* btnGoForward;
	QPushButton* btnGoBack;
	QPushButton* btnFilters;
	QPushButton* btnCreateAsset;
	QSplitter* splitter;

};
