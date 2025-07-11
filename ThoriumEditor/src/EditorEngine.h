#pragma once

#include "Engine.h"
#include "EditorCore.h"
#include "Rendering/RenderCommands.h"
#include "Object/Delegate.h"
#include "Window.h"
#include "CameraController.h"
#include "Layers/EditorLog.h"
#include "SceneOutlinerData.h"
#include "EditorAddon.h"

#include "Rendering/Framebuffer.h"

#define ITexture2D_ImGui(tex) (((DirectXTexture2D*)tex)->view)

class CLayer;
class CEntity;

class CEditorMenu;
class CPropertiesWidget;
class CConsoleWidget;
class CInputOutputWidget;
class CProjectSettingsWidget;
class CAssetBrowserWidget;
class CEditorSettingsWidget;
class CAddonsWindow;
class CEditorLogWnd;
class FClass;

class CModelComponent;
class CObjectDebugger;

struct FEditorLog;

enum ImGuiKey : int;

extern FEditorLog gBuildLog;

enum ESelectMode
{
	ESelectMode_Object,
	ESelectMode_Skeleton,
	ESelectMode_Vertices,
	ESelectMode_Faces,
	ESelectMode_Edges
};

extern FEditorLog gBuildLog;

struct SDK_API FEditorShortcut
{
public:
	FEditorShortcut(const FString& name, const FString& context, ImGuiKey key, bool shift = false, bool ctrl = false);
	
	operator bool();

	inline const FString& ToString() const { return asString; }

	void SetKey(ImGuiKey key, bool shift, bool ctrl);

	inline ImGuiKey Key() const { return key; }
	inline bool ModShift() const { return bShift; }
	inline bool ModCtrl() const { return bCtrl; }

private:
	void _SetString();

public:
	static void SaveConfig();
	static void LoadConfig();
	static TArray<FEditorShortcut*>& GetShortcuts();

	//static TArray<FEditorShortcut*> shortcuts;

	FString name;
	FString context;

	FString friendlyName;

private:
	bool bShift : 1;
	bool bCtrl : 1;
	ImGuiKey key;

	FString asString;
};

extern FEditorShortcut scSaveScene;
extern FEditorShortcut scSaveSceneAs;
extern FEditorShortcut scNewScene;
extern FEditorShortcut scOpenScene;

extern FEditorShortcut scNewProject;
extern FEditorShortcut scOpenProject;

class SDK_API CEditorEngine : public CEngine
{
public:
	void Init() override;

	int Run() override;

	void OnExit() override;

	void UpdateEditor();

	bool LoadProject(const FString& path) override;

	void MakeProject(const FProject& project);

public:
	inline static FString GetEditorConfigPath() { return OSGetDataPath() + "/ThoriumEngine/EditorConfig"; }

	void LoadEditorConfig();
	void SaveEditorConfig();

	void CompileProjectCode(int config = 0);
	void GenerateProjectSln();

	void GenerateCppClass(const FString& path, const FString& className, const FString& baseClass);

	void RegisterProject(const FProject& proj);

	bool IsEntitySelected(CEntity* ent);
	void AddSelectedEntity(CEntity* ent);
	void RemoveSelectedEntity(CEntity* ent);
	void SetSelectedEntity(CEntity* ent);

	template<typename T>
	T* AddLayer() { T* r = new T(); AddLayer(r); return r; }

	void AddLayer(TObjectPtr<CLayer> layer);
	void RemoveLayer(TObjectPtr<CLayer> layer);

	// Remove layer the next frame
	void PollRemoveLayer(TObjectPtr<CLayer> layer);

	void KeyEventA(EKeyCode key, EInputAction action, EInputMod mod);

	void SaveProjectConfig();

	void RegisterMenu(CEditorMenu* menu, const FString& path = FString());
	CEditorMenu* GetMenu(const FString& path);

	inline ESelectMode SelectMode() const { return selectMode; }
	void SetSelectMode(ESelectMode mode);

	void SetStatus(const FString& text);
	void SetStatusHighlight(const FString& text);
	void SetStatusError(const FString& text);

private:
	void InitEditorData();
	void SetupMenu();
	void SetupEditorDocking();

	void FetchEditorAddons();
	void LoadEditorAddons();

	FEditorAddon* GetEditorAddon(const FString& id);

	void LoadEditorAddon(FEditorAddon* addon);
	void UnloadEditorAddon(FEditorAddon* addon);

	inline const TArray<FEditorAddon>& GetEditorAddons() const { return editorAddons; }

	void UpdateTitle();

	void GenerateGrid(float gridSize, float quadSize, FMesh* out);

	void NewScene();
	bool SaveScene();

	void StartPlay();
	void StopPlay();

	void DoMousePick();
	void DoEntRightClick();

	// Handle asset dropping
	void DoMaterialDrop(TObjectPtr<CMaterial> mat, bool bPeek);
	void DoModelAssetDrop(TObjectPtr<CModelAsset> mdl, bool bPeek);

	void OnLevelChange();
	
	void ToggleGameInput();

	void DrawSelectionDebug();
	void FocusOnSelection();

	void DrawOutliner();
	void OutlinerDrawEntity(CEntity* ent, bool bRoot = true);
	void EntityContextMenu(CEntity* ent, const FVector& clickPos);
	void DoEntityShortcuts();

	void DoEditorRender();

	void CopyEntity();
	void PasteEntity(const FVector& pos);

	void SceneFileDialogs();

	void DupeEntity();

	void DrawObjectCreateMenu();

	void DrawMenu(CEditorMenu* m);

	void UpdateGizmos();

	void UpdateGizmoEntity();
	void UpdateGizmoSkeleton();
	//void UpdateGizmoVertex();
	//void UpdateGizmoFace();
	//void UpdateGizmoEdge();

	void DrawSelectedSkeleton();

	void MakeOutlinerTree();
	void AddOutlinerFolder(const FString& name, SizeType parent);

public:
	static void OSOpenFileManager(const FString& path);
	static void OSOpenFile(const FString& path);

	static void OSSetClipboardData(const FString& txt);
	static FString OSGetClipboardData();

public:
	IFrameBuffer* sceneFrameBuffer;
	//IDepthBuffer* sceneDepthBuffer;
	int viewportWidth, viewportHeight;
	float viewportX = 0.f, viewportY = 0.f;

	TArray<FProject> availableProjects;

	TArray<CLayer*> removeLayers;
	TArray<TObjectPtr<CLayer>> layers;

	TArray<TObjectPtr<CEntity>> selectedEntities;
	TObjectPtr<CObject> selectedObject;

	TArray<FEditorAddon> editorAddons;
	TArray<FEditorModule*> editorModules;

	FSceneOutlinerFolder outlinerRoot;
	TMap<SizeType, FEntityEditorData> entityData;

	union
	{
		int selectedBone = -1;
		int selectedVertex;
		int selectedFace;
		int selectedEdge;
	};

	// the component that the selected bone is from
	TObjectPtr<CModelComponent> boneComponent;

	bool bSelectionBoundingBox = true;
	bool bSelectionOverlay = true;
	bool bSelectionSizeText = true;
	bool bGameView = false;

	bool bGizmoActive = false;
	FMatrix manipulationMatrix;

	ESelectMode selectMode = ESelectMode_Object;

	bool bGizmoLocal = false;
	int gizmoMode = 0;

	float translateSnap = 1.f;
	float rotationSnap = 45.f;
	float scaleSnap = 1.f;
	bool bSnapTranslate = 0;
	bool bSnapRotation = 0;
	bool bSnapScale = 0;

	bool bViewportHasFocus = false;

	CEditorMenu* rootMenu;

	CCameraProxy* editorCamera;

	bool bIsPlaying = false;
	bool bPaused = false;
	bool bStepFrame = false;

	//bool bViewOutliner = true;
	//bool bViewAssetBrowser = true;
	//bool bViewStats = false;

	//bool bImGuiDemo = false;

	CEditorMenu* menuViewOutliner = nullptr;
	CEditorMenu* menuAssetBrowser = nullptr;
	CEditorMenu* menuStatistics = nullptr;

	CEditorMenu* menuImGuiDemo = nullptr;

	CEditorMenu* menuCloseProject = nullptr;
	
	CEditorMenu* menuGenProjSln = nullptr;
	CEditorMenu* menuOpenProjSln = nullptr;
	CEditorMenu* menuCompileProjCode = nullptr;
	CEditorMenu* menuCreateCppClass = nullptr;

	bool bShowProjectBrowserAtStartup = true;

	bool bOpenProj = false;

	double imguiRenderTime;
	double editorUpdateTime;

	enum EStatusType
	{
		STATUS_INFO,
		STATUS_HIGHLIGHT,
		STATUS_ERROR
	};

	int statusType = 0;
	FString curStatus;
	float statusTimer = 0.f;

	struct {
		int wndPosX = 100;
		int wndPosY = 100;
		int wndWidth = 1600;
		int wndHeight = 900;
		int wndMode = 1;
		FString theme;
	} editorCfg;

	enum ECopyBufferData
	{
		CB_NONE,
		CB_ENTITY,
		CB_ENTITY_COMPONENT
	};

	struct {
		ECopyBufferData dataType = CB_NONE;
		FMemStream data;
		FClass* type = nullptr;
	} copyBuffer;

private:
	CCameraController* camController;

	CPropertiesWidget* propertyEditor;
	CConsoleWidget* consoleWidget;
	CInputOutputWidget* ioWidget;
	CProjectSettingsWidget* projSettingsWidget;
	CAssetBrowserWidget* assetBrowser;
	CAddonsWindow* addonsWindow;
	CEditorSettingsWidget* editorSettings;
	CEditorLogWnd* logWnd;

	CObjectDebugger* objectDebuggerWidget;

	TObjectPtr<CMaterial> outlineMat;
	TObjectPtr<CMaterial> gridMat;

	FMesh boxOutlineMesh;
	FMesh gridMesh;

};

inline CEditorEngine* gEditorEngine() { return (CEditorEngine*)gEngine; }
