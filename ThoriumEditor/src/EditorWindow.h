#pragma once

#include "Editor.h"
#include <QMainWindow>
#include "DockManager.h"
#include "Gizmo.h"
#include "Windows/ToolsWindow.h"
#include "EditorConfig.h"

class CEntity;
class QMenuBar;
class QMenu;
class CEditorWindow;
class CRenderWidget;
class CViewportWidget;
class CConsoleWidget;
class QToolBar;
class QToolButton;
class QComboBox;
class QActionGroup;
class IEditorTool;
class CContentBrowserWidget;
class COutlinerWindow;
class QUndoStack;
class CPropertiesWidget;
class QTabBar;
class CMainDockWidget;

extern EDITOR_API CEditorWindow* gEditorWindow;
extern EDITOR_API CEditorVar evEditorTheme;

class EDITOR_API CEditorWindow : public CToolsWindow
{
	Q_OBJECT
	Q_DISABLE_COPY(CEditorWindow)

	ToolsWindowBody(CEditorWindow, "Editor Window", false)

public:
	CEditorWindow();
	~CEditorWindow();

	virtual bool Shutdown() override;
	virtual void SetupUi() override;

	void SetGizmoMode(EGizmoMode mode);
	inline EGizmoMode GizmoMode() const { return gizmoMode; }

	void SetTool(IEditorTool* tool);
	void SetTool(const QString& tool);
	inline IEditorTool* ActiveTool() const { return activeTool; }

	void RegisterTool(IEditorTool* tool);
	void UnregisterTool(IEditorTool* tool);

	void DoEntityContextMenu(CEntity* ent, const QPoint& pos);

	bool TrySaveScene();
	void SaveScene();

	static int ExecSaveMessageBox();
	static void LoadStyleSheet();

	static inline FString CurTheme() { return evEditorTheme.GetValue().ToString(); }
	static void SetTheme(const FString& themeName);
	static const TArray<FString>& GetAvailableThemes();

signals:
	void onGizmoModeChanged();
	void onToolChanged();
	void onSaveScene();

public slots:
	void engineUpdate();
	void levelChanged();

	void updateTitle();

	void mousePick(const FRay& ray, bool bIsRightMouse);

protected:
	void closeEvent(QCloseEvent* event) override;

	void SetupMenuBar();

	void ScanAvailableThemes();

	static void UnloadCurrentTheme();

	void UserSaveState(QSettings& out) override;
	void UserRestoreState(QSettings& in) override;

public:
	//QMenuBar* sceneMenuBar;
	//ads::CDockManager* sceneDockManager;

	// Windows
	//CMainDockWidget* sceneWnd; // the main dock widget for the scene, contains the menu and toolbars
	CConsoleWidget* consoleWindow;
	ads::CDockWidget* contentBrowser;
	CContentBrowserWidget* contentBrowserWidget;
	ads::CDockWidget* sceneDock = nullptr;
	ads::CDockWidget* gameDock = nullptr;
	COutlinerWindow* outliner = nullptr;
	ads::CDockWidget* historyDock = nullptr;
	CPropertiesWidget* propertiesWidget = nullptr;

	QUndoStack* sceneUndoStack;

	// Menu
	QMenu* menuFile;
	QMenu* menuEdit;
	QMenu* menuTools;
	QMenu* menuCode;
	QMenu* menuView;
	QMenu* menuDebug;
	QMenu* menuHelp;

	QToolBar* tbScene;
	QToolBar* tbTool;
	QToolBar* tbGizmoMode;
	QToolBar* tbGame;

	QComboBox* comboActiveTool;

	// edit actions
	QAction* actSaveScene;
	QAction* actUndo;
	QAction* actRedo;
	QAction* actCopy;
	QAction* actPaste;
	QAction* actDuplicate;
	QAction* actDelete;
	QAction* actFocusObj;
	QAction* actToggleVisObj;

	QActionGroup* actGroupGizmo;
	QAction* actGizmoSelect;
	QAction* actGizmoTranslate;
	QAction* actGizmoRotate;
	QAction* actGizmoScale;
	QAction* actGizmoBounds;

	QAction* actGamePlay;
	QAction* actGamePause;
	QAction* actGameStep;

	CViewportWidget* worldViewports[4] = { nullptr };
	CRenderWidget* gameViewport = nullptr;

private:
	IEditorTool* activeTool = nullptr;
	TArray<IEditorTool*> tools;

	EGizmoMode gizmoMode = Gizmo_Select;

};
