
#include "EditorWindow.h"
#include "System.h"
#include "EditorEngine.h"
#include "EngineThread.h"
#include "EditorTool.h"
#include "Game/World.h"
#include "Game/Entity.h"
#include "Rendering/RenderScene.h"
#include "Assets/Scene.h"
#include "Misc/FileHelper.h"
#include <Util/KeyValue.h>

#include "UndoActions/SceneUndoActions.h"

#include "MainDockWidget.h"
#include "ProjectManagerWindow.h"
#include "Windows/ConsoleWindow.h"
#include "Windows/OutlinerWindow.h"
#include "Windows/PropertiesWidget.h"
#include "Widgets/RenderWidget.h"
#include "Widgets/ContentBrowser.h"
#include "Widgets/ViewportWidget.h"
#include "Widgets/FileDialogs.h"
#include "Widgets/ClassSelectorDialog.h"
#include "Widgets/ViewportArea.h"

#include "Tools/ObjectTool.h"
#include "Tools/ModellingTool.h"

#include "DockAreaTitleBar.h"
#include <QSplashScreen>
#include <DockAreaWidget.h>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QRect>
#include <QMenuBar>
#include <QLayout>
#include <QStatusBar>
#include <QStandardPaths>
#include <QResource>
#include <QLabel>
#include <QToolBar>
#include <QToolButton>
#include <QComboBox>
#include <QActionGroup>
#include <QUndoStack>
#include <QUndoView>
#include <QMessageBox>
#include <QSpinBox>
#include <QWidgetAction>
#include <QCheckBox>

#include <filesystem>

CEditorWindow* gEditorWindow = nullptr;

SDK_REGISTER_WINDOW(CEditorWindow, "Editor Window", NULL, NULL);

CEditorVar evEditorTheme("theme", "Apearance", FVariant("default"));
static TArray<FString> availableThemes;

CEditorWindow::CEditorWindow() : CToolsWindow()
{
	gEditorWindow = this;
}

CEditorWindow::~CEditorWindow()
{
	consoleWindow->deleteLater();
	consoleWindow = nullptr;
}

bool CEditorWindow::Shutdown()
{
	if (!TrySaveScene())
		return false;

	SaveState();

	for (auto* t : tools)
		t->Shutdown();

	StopEngineThread();
	return true;
}

QWidget* MakeActionWidget(QWidget* in, const QString& label)
{
	QWidget* parent = (QWidget*)in->parent();

	QWidget* r = new QWidget(parent);
	QHBoxLayout* l = new QHBoxLayout(parent);
	r->setLayout(l);
	
	QLabel* lbl = new QLabel(label, parent);
	l->addWidget(lbl);
	l->addWidget(in);

	return r;
}

void CEditorWindow::SetupUi()
{
	ScanAvailableThemes();

	int x = QGuiApplication::primaryScreen()->geometry().width();
	int y = QGuiApplication::primaryScreen()->geometry().height();

	ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
	ads::CDockManager::setAutoHideConfigFlag(ads::CDockManager::DefaultAutoHideConfig);
	ads::CDockManager::setAutoHideConfigFlag(ads::CDockManager::AutoHideShowOnMouseOver);
	ads::CDockManager::setConfigFlag(ads::CDockManager::AllTabsHaveCloseButton);

	dockmanager = new ads::CDockManager(this);
	dockmanager->setStyleSheet("");

	QResource::registerResource(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/ThoriumEngine/EditorConfig/Themes/default_icons/icons.rcc");
	//LoadStyleSheet();
	
	x -= 1600;
	y -= 900;
	x /= 2;
	y /= 2;
	setGeometry(QRect(x, y, 1600, 900));

	sceneUndoStack = new QUndoStack(this);

	//sceneWnd = new CMainDockWidget("Scene", this);
	//sceneDockManager = new ads::CDockManager(sceneWnd->MainWindow());
	//sceneDockManager->setStyleSheet("");
	//sceneMenuBar = sceneWnd->menuBar();
	//sceneWnd->setFeatures(ads::CDockWidget::DockWidgetMovable | ads::CDockWidget::DockWidgetFocusable);

	//dockmanager->addDockWidget(ads::CenterDockWidgetArea, sceneWnd);
	
	menuFile = new QMenu("File", menuBar); menuBar->addMenu(menuFile);
	menuEdit = new QMenu("Edit", menuBar); menuBar->addMenu(menuEdit);
	menuTools = new QMenu("Tools", menuBar); menuBar->addMenu(menuTools);
	menuCode = new QMenu("Code", menuBar); menuBar->addMenu(menuCode);
	menuView = new QMenu("View", menuBar); menuBar->addMenu(menuView);
	menuDebug = new QMenu("Debug", menuBar); menuBar->addMenu(menuDebug);
	menuHelp = new QMenu("Help", menuBar); menuBar->addMenu(menuHelp);

	menuFile->addSection("Scene");
	menuFile->addAction("New Scene", this, &CEditorWindow::NewScene);
	menuFile->addAction("Open Scene");
	actSaveScene = menuFile->addAction(QIcon(":/icons/floppy.svg"), "Save", QKeySequence(Qt::CTRL | Qt::Key_S), this, &CEditorWindow::SaveScene);
	menuFile->addAction("Save As");

	menuFile->addSection("Project");
	menuFile->addAction("New Project");
	menuFile->addAction("Open Project", this, [=]() { if (close()) CToolsWindow::Create<CProjectManagerWnd>()->activateWindow(); });
	menuFile->addAction("Close Project");

	menuFile->addSection("Build");
	menuFile->addAction("Build All");
	menuFile->addAction("Build Lighting", this, [=]() { 
		if (SaveScene())
			gEditorEngine->PushEvent(EventExec_PreUpdate, []() { gEditorEngine->BakeLighting(); });
	});
	menuFile->addAction("Build Cubemaps");
	menuFile->addAction("Package Engine Content");

	menuFile->addSeparator();
	menuFile->addAction("Quit", this, [=]() { close(); });

	actUndo = sceneUndoStack->createUndoAction(this);
	actRedo = sceneUndoStack->createRedoAction(this);

	actUndo->setIcon(QIcon(":/icons/undo.svg"));
	actRedo->setIcon(QIcon(":/icons/redo.svg"));
	actUndo->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
	actRedo->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y));

	menuEdit->addAction(actUndo);
	menuEdit->addAction(actRedo);
	menuEdit->addSeparator();
	actCopy = menuEdit->addAction("Copy", QKeySequence(Qt::CTRL | Qt::Key_C));
	actPaste = menuEdit->addAction("Paste", QKeySequence(Qt::CTRL | Qt::Key_V));
	actDuplicate = menuEdit->addAction("Duplicate");
	actDelete = menuEdit->addAction("Delete", QKeySequence(Qt::Key_Delete), this, &CEditorWindow::deleteSelected);
	actFocusObj = menuEdit->addAction("Focus", QKeySequence(Qt::Key_F), this, &CEditorWindow::focusOnSelection);
	actToggleVisObj = menuEdit->addAction("Toggle Visibility", QKeySequence(Qt::Key_H), this, &CEditorWindow::toggleSelectionVis);
	menuEdit->addSeparator();
	menuEdit->addAction("Editor Settings");
	menuEdit->addAction("Project Settings");

	menuCode->addAction(QIcon(":/apps/app_visualstudio.svg"), "Generate Visual Studio Project");
	menuCode->addAction(QIcon(":/apps/app_visualstudio.svg"), "Open Visual Studio Project");
	menuCode->addAction("Compile C++ Code");

	SetupMenuBar();
	
	// Editor view
	{
		CViewportArea* area = new CViewportArea(this);

		worldViewports[0] = new CViewportWidget(this);
		worldViewports[0]->SetCamera(gEditorEngine->viewportCams[0]);
		worldViewports[0]->installEventFilter(this);
		QWidget* v1 = MakeViewportWidget(worldViewports[0]);
		area->addWidget(v1);

		worldViewports[1] = new CViewportWidget(this);
		worldViewports[1]->SetCamera(gEditorEngine->viewportCams[1]);
		worldViewports[1]->installEventFilter(this);
		QWidget* v2 = MakeViewportWidget(worldViewports[1]);
		area->addWidget(v2);

		worldViewports[2] = new CViewportWidget(this);
		worldViewports[2]->SetCamera(gEditorEngine->viewportCams[2]);
		worldViewports[2]->installEventFilter(this);
		QWidget* v3 = MakeViewportWidget(worldViewports[2]);
		area->addWidget(v3);

		worldViewports[3] = new CViewportWidget(this);
		worldViewports[3]->SetCamera(gEditorEngine->viewportCams[3]);
		worldViewports[3]->installEventFilter(this);
		QWidget* v4 = MakeViewportWidget(worldViewports[3]);
		area->addWidget(v4);

		sceneDock = new ads::CDockWidget("Scene Viewport", this);
		sceneDock->setObjectName("Scene");
		sceneDock->setWidget(area);
		dockmanager->addDockWidget(ads::CenterDockWidgetArea, sceneDock);

		area->setViewportLayout(ViewportLayout_Single);

		/*worldViewports[1] = new CViewportWidget(this);
		worldViewports[1]->SetCamera(gEditorEngine()->viewportCams[1]);
		layoutA->addWidget(worldViewports[1]);

		worldViewports[2] = new CViewportWidget(this);
		worldViewports[2]->SetCamera(gEditorEngine()->viewportCams[2]);
		layoutB->addWidget(worldViewports[2]);

		worldViewports[3] = new CViewportWidget(this);
		worldViewports[3]->SetCamera(gEditorEngine()->viewportCams[3]);
		layoutB->addWidget(worldViewports[3]);*/
	}

	// Game view
	{
		QWidget* widget = new QWidget(this);
		widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setSpacing(0);

		widget->setLayout(layout);

		gameDock = new ads::CDockWidget("Game", this);
		gameDock->setWidget(widget);
		dockmanager->addDockWidget(ads::CenterDockWidgetArea, gameDock, sceneDock->dockAreaWidget());
		sceneDock->dockAreaWidget()->setCurrentDockWidget(sceneDock);

		gameViewport = new CRenderWidget(this);
		layout->addWidget(gameViewport);
	}

	consoleWindow = new CConsoleWidget(this);
	dockmanager->addDockWidget(ads::BottomDockWidgetArea, consoleWindow);

	contentBrowser = new ads::CDockWidget("Content Browser", this);
	contentBrowser->setIcon(QIcon(":/icons/wnd_contentbrowser.svg"));
	contentBrowser->setObjectName("contentbrowser_dockwidget");
	contentBrowserWidget = new CContentBrowserWidget();
	contentBrowser->setWidget(contentBrowserWidget);
	dockmanager->addDockWidget(ads::CenterDockWidgetArea, contentBrowser, consoleWindow->dockAreaWidget());

	outliner = new COutlinerWindow(this);
	dockmanager->addDockWidget(ads::RightDockWidgetArea, outliner);

	propertiesWidget = new CPropertiesWidget(this);
	dockmanager->addDockWidget(ads::BottomDockWidgetArea, propertiesWidget, outliner->dockAreaWidget());

	{
		QWidget* widget = new QWidget(this);
		widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setSpacing(0);

		widget->setLayout(layout);

		historyDock = new ads::CDockWidget("History", this);
		historyDock->setWidget(widget);
		dockmanager->addDockWidget(ads::CenterDockWidgetArea, historyDock, outliner->dockAreaWidget());

		QUndoView* view = new QUndoView(sceneUndoStack, this);
		layout->addWidget(view);
	}

	outliner->setAsCurrentTab();

	QStatusBar* statusBar = new QStatusBar(this);
	setStatusBar(statusBar);

	{
		tbGrid = addToolBar("Grid");
		
		actShowGrid = tbGrid->addAction(QIcon(":/icons/grid.png"), "View Grid");
		actShowGrid->setCheckable(true);
		actShowGrid->setChecked(gEditorEngine->bDrawGrid);
		
		actGridSnap = tbGrid->addAction(QIcon(":/icons/snap-to-grid.png"), "Snap to Grid");
		actGridSnap->setCheckable(true);
		actShowGrid->setChecked(gEditorEngine->bGridSnap);

		QDoubleSpinBox* spinGridSize = new QDoubleSpinBox();
		spinGridSize->setRange(0.1, 100);
		spinGridSize->setValue(1.0);
		tbGrid->addWidget(spinGridSize);

		connect(spinGridSize, &QDoubleSpinBox::valueChanged, this, [=](double v) { gEditorEngine->gridSize = v; });

		actAngleSnap = tbGrid->addAction(QIcon(":/icons/angle-snap.png"), "Angle Snap");
		actAngleSnap->setCheckable(true);
		actShowGrid->setChecked(gEditorEngine->bAngleSnap);

		QDoubleSpinBox* spinRotSnap = new QDoubleSpinBox();
		spinRotSnap->setRange(0.1, 90); 
		spinRotSnap->setValue(15);
		tbGrid->addWidget(spinRotSnap);

		connect(spinRotSnap, &QDoubleSpinBox::valueChanged, this, [=](double v) { gEditorEngine->angleSnap = v; });
	}
	{
		tbScene = addToolBar("Scene");

		tbScene->addAction(actSaveScene);
		tbScene->addAction(actUndo);
		tbScene->addAction(actRedo);
	}
	{
		tbTool = addToolBar("Tool");

		comboActiveTool = new QComboBox();
		tbTool->addWidget(comboActiveTool);
		comboActiveTool->setMinimumWidth(126);
	}
	{
		tbGizmoMode = addToolBar("Gizmo Mode");

		tbGizmoMode->addAction(QIcon(":/icons/translate-worldspace.png"), "World Space")->setCheckable(true);
		tbGizmoMode->addSeparator();

		actGizmoSelect = tbGizmoMode->addAction(QIcon(":/icons/select-cursor.svg"), "Select", QKeySequence(Qt::SHIFT | Qt::Key_S));
		actGizmoSelect->setCheckable(true);
		actGizmoSelect->setChecked(true);

		actGizmoTranslate = tbGizmoMode->addAction(QIcon(":/icons/select-translate.svg"), "Translate", QKeySequence(Qt::Key_W));
		actGizmoTranslate->setCheckable(true);
			
		actGizmoRotate = tbGizmoMode->addAction(QIcon(":/icons/select-rotate.svg"), "Rotate", QKeySequence(Qt::Key_E));
		actGizmoRotate->setCheckable(true);

		actGizmoScale = tbGizmoMode->addAction(QIcon(":/icons/select-scale.svg"), "Scale", QKeySequence(Qt::Key_R));
		actGizmoScale->setCheckable(true);

		actGroupGizmo = new QActionGroup(this);
		actGroupGizmo->addAction(actGizmoSelect);
		actGroupGizmo->addAction(actGizmoTranslate);
		actGroupGizmo->addAction(actGizmoRotate);
		actGroupGizmo->addAction(actGizmoScale);
	}
	{
		tbView = addToolBar("Editor View");

		actDrawBounds = tbView->addAction(QIcon(":/icons/bounding-box.png"), "Selection Bounding Box"); actDrawBounds->setCheckable(true);
		actDrawOverlay = tbView->addAction(QIcon(":/icons/select-overlay.png"), "Selection Overlay"); actDrawOverlay->setCheckable(true);
		actDrawGizmos = tbView->addAction(QIcon(":/icons/view-gizmos.png"), "View Gizmos"); actDrawGizmos->setCheckable(true);
		actDrawBounds->setChecked(gEditorEngine->bSelectionBoundingBox);
		actDrawOverlay->setChecked(gEditorEngine->bSelectionOverlay);
		actDrawGizmos->setChecked(gEditorEngine->bDrawGizmos);
	}
	{
		tbGame = addToolBar("Play in Editor");

		actGamePlay = tbGame->addAction(QIcon(":/icons/btn-play.svg"), "A");
		actGamePause = tbGame->addAction(QIcon(":/icons/btn-pause.svg"), "B");
		actGameStep = tbGame->addAction(QIcon(":/icons/btn-stepframe.svg"), "C");
	}

	menuView->addAction(sceneDock->toggleViewAction());
	menuView->addAction(gameDock->toggleViewAction());
	menuView->addAction(consoleWindow->toggleViewAction());
	menuView->addAction(contentBrowser->toggleViewAction());
	menuView->addAction(outliner->toggleViewAction());
	menuView->addAction(historyDock->toggleViewAction());
	menuView->addAction(propertiesWidget->toggleViewAction());
	menuView->addSeparator();
	menuView->addAction("Reload Style", this, [=]() { LoadStyleSheet(); });

	RegisterTool(new CObjectTool());
	RegisterTool(new CModellingTool());

	connect(gEngineThread, &CEngineThread::onUpdate, this, &CEditorWindow::engineUpdate);
	connect(gEngineThread, &CEngineThread::onLevelChanged, this, &CEditorWindow::levelChanged);
	connect(comboActiveTool, &QComboBox::currentTextChanged, this, [=](const QString& txt) { SetTool(txt); });

	connect(actShowGrid, &QAction::triggered, this, [=](bool v) { gEditorEngine->bDrawGrid = v; });
	connect(actGridSnap, &QAction::triggered, this, [=](bool v) { gEditorEngine->bGridSnap = v; });
	connect(actAngleSnap, &QAction::triggered, this, [=](bool v) { gEditorEngine->bAngleSnap = v; });

	connect(actDrawBounds, &QAction::triggered, this, [=](bool v) { gEditorEngine->bSelectionBoundingBox = v; });
	connect(actDrawOverlay, &QAction::triggered, this, [=](bool v) { gEditorEngine->bSelectionOverlay = v; });
	connect(actDrawGizmos, &QAction::triggered, this, &CEditorWindow::showGizmos);

	connect(actGizmoSelect, &QAction::triggered, this, [=](bool b) { if (b) SetGizmoMode(Gizmo_Select); });
	connect(actGizmoTranslate, &QAction::triggered, this, [=](bool b) { if (b) SetGizmoMode(Gizmo_Translate); });
	connect(actGizmoRotate, &QAction::triggered, this, [=](bool b) { if (b) SetGizmoMode(Gizmo_Rotate); });
	connect(actGizmoScale, &QAction::triggered, this, [=](bool b) { if (b) SetGizmoMode(Gizmo_Scale); });
	
	connect(worldViewports[0], &CViewportWidget::onMousePick, this, &CEditorWindow::mousePick);
	connect(worldViewports[1], &CViewportWidget::onMousePick, this, &CEditorWindow::mousePick);
	connect(worldViewports[2], &CViewportWidget::onMousePick, this, &CEditorWindow::mousePick);
	connect(worldViewports[3], &CViewportWidget::onMousePick, this, &CEditorWindow::mousePick);

	connect(sceneUndoStack, &QUndoStack::cleanChanged, this, [=]() {
		updateTitle();

		if (gWorld->GetScene())
			gWorld->GetScene()->MarkAsDirty(!sceneUndoStack->isClean());
	});

	RestoreState();

	SetTool("Object Tool");
	showGizmos(gEditorEngine->bDrawGizmos);

	updateTitle();
}

QWidget* CEditorWindow::MakeViewportWidget(CViewportWidget* viewport)
{
	QWidget* widget = new QWidget(this);
	widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	// scene menu bar
	QVBoxLayout* l1 = new QVBoxLayout(this);
	QMenuBar* sceneMenuBar = new QMenuBar(this);
	l1->addWidget(sceneMenuBar);
	l1->setContentsMargins(0, 2, 0, 0);
	l1->setSpacing(0);
	sceneMenuBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	sceneMenuBar->setFixedHeight(32);
	sceneMenuBar->setNativeMenuBar(false);

	{
		QMenu* menuRender = sceneMenuBar->addMenu(QIcon(":/icons/rview-lit.svg"), "View Mode");
		//QAction* actRender = tb->addAction(QIcon(":/icons/rview-lit.svg"), "View Mode");
		
		QAction* actions[] = {
			menuRender->addAction(QIcon(":/icons/rview-lit.svg"), "Lit"),
			menuRender->addAction(QIcon(":/icons/rview-unlit.svg"), "Unlit"),
			menuRender->addAction(QIcon(":/icons/rview-diffuse.svg"), "Diffuse"),
			menuRender->addAction(QIcon(":/icons/rview-wireframe.svg"), "Wireframe"),
			menuRender->addAction(QIcon(":/icons/rview-normal.svg"), "Normal")
		};
		QActionGroup* group = new QActionGroup(this);

		for (int i = 0; i < 5; i++)
		{
			QAction* act = actions[i];
			act->setCheckable(true);
			group->addAction(act);

			connect(act, &QAction::triggered, this, [=](bool b) {
				if (!b)
					return;

				menuRender->setIcon(act->icon());
				viewport->GetCamera()->viewMode = (CCameraProxy::EViewMode)i;
			});

			connect(menuView, &QMenu::aboutToShow, this, [=]() {
				int v = (int)viewport->GetCamera()->viewMode;
				actions[v]->setChecked(true);
			});
		}
	}

	// Menu View
	{
		QMenu* menuView = sceneMenuBar->addMenu("View");

		QAction* actions[] = {
			menuView->addAction("Perspective"),
			menuView->addAction("Orthographic"),
			menuView->addAction("2D Top"),
			menuView->addAction("2D Front"),
			menuView->addAction("2D Side")
		};
		QActionGroup* group = new QActionGroup(this);

		for (int i = 0; i < 5; i++)
		{
			actions[i]->setCheckable(true);
			group->addAction(actions[i]);

			connect(actions[i], &QAction::triggered, this, [=](bool b) { if (b) viewport->SetViewMode((ECameraView)i); });
		}

		connect(menuView, &QMenu::aboutToShow, this, [=]() {
			int v = viewport->GetViewMode();
			actions[v]->setChecked(true);
		});
	}

	// Menu Camera
	{
		QMenu* menuCamera = sceneMenuBar->addMenu("Camera");

		QSlider* sliderFov = new QSlider(Qt::Horizontal, this);
		sliderFov->setRange(1, 179);

		QWidget* wFov = MakeActionWidget(sliderFov, "FOV");

		QWidgetAction* wa = new QWidgetAction(this);
		wa->setDefaultWidget(wFov);
		menuCamera->addAction(wa);

		QDoubleSpinBox* spinNearPlane = new QDoubleSpinBox(this);
		spinNearPlane->setRange(0.01, 100.0);
		spinNearPlane->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
		spinNearPlane->setSingleStep(0.1);

		QWidget* wNearPlane = MakeActionWidget(spinNearPlane, "Near Plane");

		wa = new QWidgetAction(this);
		wa->setDefaultWidget(wNearPlane);
		menuCamera->addAction(wa);

		QDoubleSpinBox* spinFarPlane = new QDoubleSpinBox(this);
		spinFarPlane->setRange(0.1, 10000.0);
		spinFarPlane->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
		spinFarPlane->setSingleStep(0.1);

		QWidget* wFarPlane = MakeActionWidget(spinFarPlane, "Far Plane");

		wa = new QWidgetAction(this);
		wa->setDefaultWidget(wFarPlane);
		menuCamera->addAction(wa);

		QCheckBox* checkWireframe = new QCheckBox(this);

		QWidget* wWireframe = MakeActionWidget(checkWireframe, "Wireframe");

		wa = new QWidgetAction(this);
		wa->setDefaultWidget(wWireframe);
		menuCamera->addAction(wa);

		connect(sliderFov, &QSlider::valueChanged, this, [=](int v) { viewport->camFov = (float)v; });
		connect(spinNearPlane, &QDoubleSpinBox::valueChanged, this, [=](double v) { viewport->GetCamera()->nearPlane = (float)v; });
		connect(spinFarPlane, &QDoubleSpinBox::valueChanged, this, [=](double v) { viewport->GetCamera()->farPlane = (float)v; });
		connect(checkWireframe, &QCheckBox::stateChanged, this, [=](int v) { viewport->GetCamera()->bDrawWireframe = v == Qt::Checked; });

		connect(menuCamera, &QMenu::aboutToShow, this, [=]() {
			sliderFov->setValue((int)viewport->camFov);
			spinNearPlane->setValue(viewport->GetCamera()->nearPlane);
			spinFarPlane->setValue(viewport->GetCamera()->farPlane);
			checkWireframe->setChecked(viewport->GetCamera()->bDrawWireframe);
		});
	}

	l1->addWidget(viewport);
	widget->setLayout(l1);
	return widget;
}

bool CEditorWindow::eventFilter(QObject* obj, QEvent* ev)
{
	for (int i = 0; i < 4; i++)
	{
		if (obj == worldViewports[i])
		{
			if (ev->type() == QEvent::MouseMove)
				activeViewport = worldViewports[i];
			//else if (ev->type() == QEvent::HoverLeave && activeViewport == obj)
			//	activeViewport = nullptr;

			if (activeTool && activeTool->viewportEvent(obj, ev))
				return true;
		}
	}
	return false;
}

void CEditorWindow::showEvent(QShowEvent* ev)
{
	if (gSplashscreen)
		gSplashscreen->finish(this);

	CToolsWindow::showEvent(ev);
}

bool CEditorWindow::event(QEvent* e)
{
	if (e->type() == EditorEvents_ThreadEvent)
	{
		auto* ev = (FThreadEvent*)e;
		ev->Invoke();
		return true;
	}

	return CToolsWindow::event(e);
}

void CEditorWindow::engineUpdate()
{
	if (activeTool)
		activeTool->Update();
}

void CEditorWindow::SetGizmoMode(EGizmoMode mode)
{
	gizmoMode = mode;
	emit onGizmoModeChanged(mode);
}

void CEditorWindow::SetTool(IEditorTool* tool)
{
	if (activeTool == tool)
		return;

	if (activeTool)
	{
		activeTool->Disable();
		emit activeTool->onDisabled();
	}

	activeTool = tool;
	activeTool->Enable();
	emit activeTool->onEnabled();

	comboActiveTool->setCurrentIndex(comboActiveTool->findText(activeTool->objectName()));
}

void CEditorWindow::SetTool(const QString& tool)
{
	for (auto t : tools)
	{
		if (t->objectName() == tool)
		{
			SetTool(t);
			return;
		}
	}
}

void CEditorWindow::RegisterTool(IEditorTool* tool)
{
	tools.Add(tool);
	tool->Init();

	comboActiveTool->addItem(tool->getIcon(), tool->objectName());
}

void CEditorWindow::UnregisterTool(IEditorTool* tool)
{
	tools.Erase(tools.Find(tool));
}

void CEditorWindow::DoEntityContextMenu(CEntity* ent, const QPoint& pos)
{
	QMenu menu(this);

	menu.addAction(actCopy);
	menu.addAction(actPaste);
	menu.addAction(actDuplicate);
	menu.addAction(actDelete);
	menu.addAction(actFocusObj);
	menu.addAction(actToggleVisObj);
	
	menu.addSeparator();

	QMenu* attach = menu.addMenu("Attach To...");
	auto ents = gWorld->GetEntities();
	for (auto e : ents)
	{
		if (e.second == ent)
			continue;

		attach->addAction(e.second->Name().c_str(), this, [=]() {
			auto* prev = ent->RootComponent()->GetParent();
			ent->RootComponent()->AttachTo(e.second->RootComponent());

			sceneUndoStack->push(new CmdReparentComponent(ent->RootComponent(), prev));
		});
	}

	QAction* detach = menu.addAction("Detach from Parent", this, [=]() { 
		auto* prev = ent->RootComponent()->GetParent();
		ent->RootComponent()->Detach(); 

		sceneUndoStack->push(new CmdReparentComponent(ent->RootComponent(), prev));
	});
	detach->setEnabled(ent->RootComponent()->GetParent() != nullptr);

	menu.exec(pos);
}

void CEditorWindow::levelChanged()
{
	sceneUndoStack->clear();

	propertiesWidget->SetObject(nullptr);
	updateTitle();
}

void CEditorWindow::updateTitle()
{
	if (!gWorld)
	{
		setWindowTitle("Thorium Editor");
		return;
	}

	CScene* scene = gWorld->GetScene();
	if (scene && scene->File())
	{
		QString name(scene->File()->Path().c_str());
		if (!sceneUndoStack->isClean())
			name += "*";

		setWindowTitle("Thorium Editor - " + name);
	}
	else
	{
		QString name("new scene");
		if (!sceneUndoStack->isClean())
			name += "*";

		setWindowTitle("Thorium Editor - " + name);
	}
}

void CEditorWindow::mousePick(const FRay& ray, bool bIsRightMouse)
{
	auto* scene = gWorld->GetRenderScene();

	FPrimitiveHitInfo hit;

	bool bHit = scene->RayCast(ray.origin, ray.direction, &hit);

	if (!bIsRightMouse)
	{
		if (bHit)
		{
			CEntity* ent = nullptr;

			TObjectPtr<CObject> obj = hit.hitProxy->GetOwner();
			if (auto comp = CastChecked<CSceneComponent>(obj); comp)
			{
				ent = comp->GetEntity();
			}

			if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier)
			{
				if (gEditorEngine->IsObjectSelected(ent))
				{
					if (gEditorEngine->activeObject == ent)
						gEditorEngine->RemoveSelectedObject(ent);
					else
						gEditorEngine->activeObject = ent;
				}
				else
					gEditorEngine->AddSelectedObject(ent);
			}
			else
				gEditorEngine->SelectObject(ent);
		}
		else
			gEditorEngine->ClearSelection();
	}
	else if (bHit)
	{
		CEntity* ent = nullptr;

		TObjectPtr<CObject> obj = hit.hitProxy->GetOwner();
		if (auto comp = CastChecked<CSceneComponent>(obj); comp)
			ent = comp->GetEntity();

		if (ent)
			DoEntityContextMenu(ent, QCursor::pos());
	}
}

void CEditorWindow::deleteSelected()
{
	auto selected = gEditorEngine->selectedObjects;
	for (auto obj : selected)
	{
		// we only allow deleting of entities for now.
		if (auto ent = CastChecked<CEntity>(obj); ent)
		{
			if (obj == gEditorEngine->activeObject)
				gEditorEngine->activeObject = nullptr;

			gEditorEngine->RemoveSelectedObject(obj);
			obj->Delete();

			// TODO: add undo for this.
		}
	}
}

void CEditorWindow::hideGizmos()
{
	showGizmos(false);
}

void CEditorWindow::showGizmos(bool v)
{
	for (int i = 0; i < 4; i++)
	{
		if (worldViewports[i])
		{
			auto* cam = worldViewports[i]->GetCamera();
			if (v)
				cam->layers = (ERenderLayer)(cam->layers | R_LAYER_EDITOR);
			else
				cam->layers = (ERenderLayer)(cam->layers & (~R_LAYER_EDITOR));
		}
	}
	gEditorEngine->bDrawGizmos = v;
}

void CEditorWindow::toggleSelectionVis()
{
	auto ents = gEditorEngine->GetSelectedObjects<CEntity>();
	if (ents.Size() == 0)
		return;

	bool bVis = ents[0]->bIsVisible ^ 1;
	for (auto ent : ents)
		ent->bIsVisible = bVis;
}

void CEditorWindow::focusOnSelection()
{
	auto ents = gEditorEngine->GetSelectedObjects<CEntity>();
	if (ents.Size() == 0)
		return;

	FBounds b ;
	for (auto* ent : ents)
		b = b.Combine(ent->GetBounds());

	for (int i = 0; i < 4; i++)
	{
		if (!worldViewports[i] || !worldViewports[i]->GetCamera())
			continue;
		
		// only affect perspective viewports
		if (worldViewports[i]->GetViewMode() != Cam3DPerspective)
			continue;

		auto* cam = worldViewports[i]->GetCamera();
		cam->position = b.position - cam->GetForwardVector() * FMath::Max(b.extents.Magnitude() * 1.5f, 1.f);
	}
}

void CEditorWindow::closeEvent(QCloseEvent* event)
{
	if (!gIsRunning || CToolsWindow::CloseAll(this))
	{
		if (!Shutdown())
		{
			event->ignore();
			return;
		}

		event->accept();
	}
	else
		event->ignore();
}

void CEditorWindow::SetupMenuBar()
{
	for (auto w : ToolsRegisteredWindows::Get())
	{
		if (w->Id == GetId() || !w->ToolBarPath)
			continue;

		// Split the ToolBarPath with '/' or '\\'
		auto split = FString(w->ToolBarPath).Split("/\\");

		QMenu* curMenu = nullptr;
		for (auto* m : menuBar->children())
		{
			QMenu* menu = qobject_cast<QMenu*>(m);
			if (menu && menu->title() == split[0].c_str())
			{
				curMenu = menu;
				break;
			}
		}
		if (curMenu == nullptr)
		{
			curMenu = new QMenu(split[0].c_str(), menuBar);
			curMenu->setObjectName(split[0].c_str());
			//curMenu->setTitle();
			menuBar->addMenu(curMenu);
		}

		for (auto p = split.begin()++; p != split.end(); p++)
		{
			if (QMenu* m = curMenu->findChild<QMenu*>(p->c_str())) {
				curMenu = m;
				continue;
			}

			QMenu* prevMenu = curMenu;
			curMenu = new QMenu(prevMenu);
			curMenu->setObjectName(p->c_str());
			curMenu->setTitle(p->c_str());
			prevMenu->addMenu(curMenu);
		}

		QAction* action = nullptr;
		for (auto a : curMenu->actions())
		{
			if (a->text() == w->Name)
			{
				action = a;
				break;
			}
		}

		if (!action)
		{
			action = new QAction(w->Name, this);
			curMenu->addAction(action);
		}
		
		action->setObjectName(w->Name);
		if (w->icon)
		{
			action->setIcon(*w->icon);
			action->setIconText(w->Name);
		}

		connect(action, &QAction::triggered, this, [=](bool) { w->Create(); });
	}
}

void CEditorWindow::ScanAvailableThemes()
{
	FString enginePath = SSystem::GetEnginePath();

	QString themePath = (enginePath + "/content/editor/themes/").c_str();
	availableThemes.Clear();

	for (auto& entry : std::filesystem::directory_iterator(themePath.toStdString()))
	{
		if (FFileHelper::FileExists((themePath + entry.path().filename().c_str() + "/theme").toStdString().c_str()))
		{
			availableThemes.Add(entry.path().filename().string().c_str());
		}
	}
}

bool CEditorWindow::TrySaveScene()
{
	if (!sceneUndoStack->isClean())
	{
		int r = ExecSaveMessageBox();
		if (r == QMessageBox::Cancel)
			return false;

		if (r == QMessageBox::Save)
			SaveScene();
	}
	return true;
}

bool CEditorWindow::SaveScene()
{
	if (!gWorld->GetScene()->File())
	{
		//ThoriumEditor::SaveFile("saveEditorScene", (FAssetClass*)CScene::StaticClass());
		CSaveFileDialog dialog(this);
		if (!dialog.exec())
			return false;

		if (!CAssetManager::RegisterNewAsset(gWorld->GetScene(), dialog.Path(), dialog.Mod()))
			return false;
	}

	gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() { gEditorEngine->OnSaveScene(); });

	sceneUndoStack->setClean();
	emit onSaveScene();
	return true;
}

void CEditorWindow::SaveSceneAs()
{
	// TODO: Implement SaveSceneAs
}

void CEditorWindow::OpenScene()
{
	if (!TrySaveScene())
		return;

	COpenFileDialog dialog((FAssetClass*)CScene::StaticClass(), this);
	if (!dialog.exec())
		return;

	if (dialog.File())
	{
		FFile* file = dialog.File();
		gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
			gEditorEngine->LoadWorld(file->Path());
		});
	}
}

void CEditorWindow::NewScene()
{
	if (!TrySaveScene())
		return;
	
	gEditorEngine->PushEvent(EventExec_PreUpdate, []() {
		gEditorEngine->LoadWorld();
	});
}

void CEditorWindow::CreateEntityPopup(FClass* base, const FString& name, const FTransform& transform, std::function<void(CEntity*)> createCallback)
{
	CClassSelectorDialog dialog(this);
	if (!base)
		base = CEntity::StaticClass();

	dialog.SetFilterClass(base);
	if (dialog.exec())
	{
		FClass* type = dialog.GetSelectedClass();

		//CEntity* ent = Cast<CEntity>(type->Instantiate());
		gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
			CEntity* ent = gWorld->CreateEntity(type, name);
			ent->SetPosition(transform.position);
			ent->SetRotation(transform.rotation);
			ent->SetScale(transform.scale);
			if (createCallback)
				createCallback(ent);
		});
	}
}

void CEditorWindow::CreateEntity(FClass* type, const FString& name, const FTransform& transform, std::function<void(CEntity*)> createCallback)
{
	gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
		CEntity* ent = gWorld->CreateEntity(type, name);
		ent->SetPosition(transform.position);
		ent->SetRotation(transform.rotation);
		ent->SetScale(transform.scale);
		if (createCallback)
			createCallback(ent);
	});
}

int CEditorWindow::ExecSaveMessageBox()
{
	QMessageBox msg;
	msg.setText("Do you want to save before closing?");
	FString name = "new scene";
	if (gWorld->GetScene())
		name = gWorld->GetScene()->File()->Path();

	msg.setInformativeText(name.c_str());
	msg.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
	msg.setDefaultButton(QMessageBox::Save);
	return msg.exec();
}

#include <fstream>

void CEditorWindow::LoadStyleSheet()
{
	QString styleSheet;

	FString enginePath = SSystem::GetEnginePath();

	QString themePath = (enginePath + "/content/editor/themes/" + CurTheme() + "/").c_str();
	QString themeFilePath = themePath + "theme";
	FKeyValue theme(themeFilePath.toUtf8().constData());
	THORIUM_ASSERT(theme.IsOpen(), FString("Failed to open theme file '") + (const char*)themeFilePath.toUtf8().constData() + "'");

	if (!theme.IsOpen())
	{
		qApp->setStyle("fusion");
		return;
	}

	for (auto& v : *theme.GetArray("include", true))
	{
		std::ifstream includeStream((themePath.toUtf8().constData() + v).c_str());
		THORIUM_ASSERT(includeStream.is_open(), FString("Failed to open '") + v + "'");
		std::string _l;	
		while (std::getline(includeStream, _l))
			styleSheet += _l + '\n';
	}

	qApp->setStyleSheet(styleSheet);

	QResource::registerResource(themePath + "icons.rcc");
}

void CEditorWindow::UnloadCurrentTheme()
{
	QResource::unregisterResource((SSystem::GetEnginePath() + "/content/editor/themes/" + CurTheme() + "/icons.rcc").c_str());
}

void CEditorWindow::UserSaveState(QSettings& out)
{
	//out.setValue("sceneDockManager", sceneDockManager->saveState());
}

void CEditorWindow::UserRestoreState(QSettings& in)
{
	//sceneDockManager->restoreState(in.value("sceneDockManager").toByteArray());
}

void CEditorWindow::SetTheme(const FString& themeName)
{
	for (const auto& t : availableThemes)
	{
		if (t == themeName)
		{
			UnloadCurrentTheme();

			evEditorTheme.SetValue(FVariant(themeName));
			LoadStyleSheet();
			return;
		}
	}
}

const TArray<FString>& CEditorWindow::GetAvailableThemes()
{
	return availableThemes;
}

FThreadEvent::FThreadEvent(std::function<void()> f) : QEvent((QEvent::Type)EditorEvents_ThreadEvent), func(f)
{
}

void FThreadEvent::Invoke()
{
	func();
}
