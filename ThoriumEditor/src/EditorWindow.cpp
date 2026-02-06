
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

void CEditorWindow::SetupUi()
{
	if (gSplashscreen)
		gSplashscreen->finish(this);

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

	sceneWnd = new CMainDockWidget("Scene", this);
	sceneDockManager = new ads::CDockManager(sceneWnd->MainWindow());
	sceneDockManager->setStyleSheet("");
	sceneMenuBar = sceneWnd->menuBar();
	sceneWnd->setFeatures(ads::CDockWidget::DockWidgetMovable | ads::CDockWidget::DockWidgetFocusable);

	dockmanager->addDockWidget(ads::CenterDockWidgetArea, sceneWnd);
	sceneWnd->dockAreaWidget()->titleBar()->insertWidget(0, new QLabel("Thorium Editor"));

	menuFile = new QMenu("File", sceneMenuBar); sceneMenuBar->addMenu(menuFile);
	menuEdit = new QMenu("Edit", sceneMenuBar); sceneMenuBar->addMenu(menuEdit);
	menuTools = new QMenu("Tools", sceneMenuBar); sceneMenuBar->addMenu(menuTools);
	menuCode = new QMenu("Code", sceneMenuBar); sceneMenuBar->addMenu(menuCode);
	menuView = new QMenu("View", sceneMenuBar); sceneMenuBar->addMenu(menuView);
	menuDebug = new QMenu("Debug", sceneMenuBar); sceneMenuBar->addMenu(menuDebug);
	menuHelp = new QMenu("Help", sceneMenuBar); sceneMenuBar->addMenu(menuHelp);

	menuFile->addSection("Scene");
	menuFile->addAction("New Scene");
	menuFile->addAction("Open Scene");
	actSaveScene = menuFile->addAction(QIcon(":/icons/floppy.svg"), "Save");
	menuFile->addAction("Save As");

	menuFile->addSection("Project");
	menuFile->addAction("New Project");
	menuFile->addAction("Open Project", this, [=]() { if (close()) CToolsWindow::Create<CProjectManagerWnd>()->activateWindow(); });
	menuFile->addAction("Close Project");

	menuFile->addSection("Build");
	menuFile->addAction("Build All");
	menuFile->addAction("Build Lighting");
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
	actDelete = menuEdit->addAction("Delete", QKeySequence(Qt::Key_Delete));
	actFocusObj = menuEdit->addAction("Focus", QKeySequence(Qt::Key_F));
	actToggleVisObj = menuEdit->addAction("Toggle Visibility", QKeySequence(Qt::Key_H));
	menuEdit->addSeparator();
	menuEdit->addAction("Editor Settings");
	menuEdit->addAction("Project Settings");

	menuCode->addAction(QIcon(":/apps/app_visualstudio.svg"), "Generate Visual Studio Project");
	menuCode->addAction(QIcon(":/apps/app_visualstudio.svg"), "Open Visual Studio Project");
	menuCode->addAction("Compile C++ Code");

	SetupMenuBar();
	
	// Editor view
	{
		QWidget* widget = new QWidget(this);
		widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		QHBoxLayout* layout = new QHBoxLayout(this);
		//layout->setSpacing(0);

		QVBoxLayout* layoutA = new QVBoxLayout(this);
		layout->addLayout(layoutA);
		QVBoxLayout* layoutB = new QVBoxLayout(this);
		layout->addLayout(layoutB);

		widget->setLayout(layout);

		sceneDock = new ads::CDockWidget("Scene Viewport", this);
		sceneDock->setObjectName("Scene");
		sceneDock->setWidget(widget);
		sceneDockManager->addDockWidget(ads::CenterDockWidgetArea, sceneDock);

		worldViewports[0] = new CViewportWidget(this);
		worldViewports[0]->SetCamera(gEditorEngine->viewportCams[0]);
		layoutA->addWidget(worldViewports[0]);

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
		sceneDockManager->addDockWidget(ads::CenterDockWidgetArea, gameDock, sceneDock->dockAreaWidget());
		sceneDock->dockAreaWidget()->setCurrentDockWidget(sceneDock);

		gameViewport = new CRenderWidget(this);
		layout->addWidget(gameViewport);
	}

	consoleWindow = new CConsoleWidget(this);
	sceneDockManager->addDockWidget(ads::BottomDockWidgetArea, consoleWindow);

	contentBrowser = new ads::CDockWidget("Content Browser", this);
	contentBrowser->setIcon(QIcon(":/icons/wnd_contentbrowser.svg"));
	contentBrowser->setObjectName("contentbrowser_dockwidget");
	contentBrowserWidget = new CContentBrowserWidget();
	contentBrowser->setWidget(contentBrowserWidget);
	sceneDockManager->addDockWidget(ads::CenterDockWidgetArea, contentBrowser, consoleWindow->dockAreaWidget());

	outliner = new COutlinerWindow(this);
	sceneDockManager->addDockWidget(ads::RightDockWidgetArea, outliner);

	propertiesWidget = new CPropertiesWidget(this);
	sceneDockManager->addDockWidget(ads::BottomDockWidgetArea, propertiesWidget, outliner->dockAreaWidget());

	{
		QWidget* widget = new QWidget(this);
		widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setSpacing(0);

		widget->setLayout(layout);

		historyDock = new ads::CDockWidget("History", this);
		historyDock->setWidget(widget);
		sceneDockManager->addDockWidget(ads::CenterDockWidgetArea, historyDock, outliner->dockAreaWidget());

		QUndoView* view = new QUndoView(sceneUndoStack, this);
		layout->addWidget(view);
	}

	outliner->setAsCurrentTab();

	QStatusBar* statusBar = new QStatusBar(this);
	setStatusBar(statusBar);

	{
		tbScene = sceneWnd->addToolBar("Scene");

		tbScene->addAction(actSaveScene);
		tbScene->addAction(actUndo);
		tbScene->addAction(actRedo);
	}
	{
		tbTool = sceneWnd->addToolBar("Tool");

		comboActiveTool = new QComboBox();
		tbTool->addWidget(comboActiveTool);
		comboActiveTool->setMinimumWidth(126);
	}
	{
		tbGizmoMode = sceneWnd->addToolBar("Gizmo Mode");

		actGizmoSelect = tbGizmoMode->addAction(QIcon(":/icons/select-cursor.svg"), "Select");
		actGizmoSelect->setCheckable(true);
		actGizmoSelect->setChecked(true);

		actGizmoTranslate = tbGizmoMode->addAction(QIcon(":/icons/select-translate.svg"), "Translate");
		actGizmoTranslate->setCheckable(true);
			
		actGizmoRotate = tbGizmoMode->addAction(QIcon(":/icons/select-rotate.svg"), "Rotate");
		actGizmoRotate->setCheckable(true);

		actGizmoScale = tbGizmoMode->addAction(QIcon(":/icons/select-scale.svg"), "Scale");
		actGizmoScale->setCheckable(true);

		actGroupGizmo = new QActionGroup(this);
		actGroupGizmo->addAction(actGizmoSelect);
		actGroupGizmo->addAction(actGizmoTranslate);
		actGroupGizmo->addAction(actGizmoRotate);
		actGroupGizmo->addAction(actGizmoScale);
	}
	{
		tbGame = sceneWnd->addToolBar("Play in Editor");

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

	connect(actGizmoSelect, &QAction::triggered, this, [=](bool b) { if (b) SetGizmoMode(Gizmo_Select); });
	connect(actGizmoTranslate, &QAction::triggered, this, [=](bool b) { if (b) SetGizmoMode(Gizmo_Translate); });
	connect(actGizmoRotate, &QAction::triggered, this, [=](bool b) { if (b) SetGizmoMode(Gizmo_Rotate); });
	connect(actGizmoScale, &QAction::triggered, this, [=](bool b) { if (b) SetGizmoMode(Gizmo_Scale); });
	
	connect(worldViewports[0], &CViewportWidget::onMousePick, this, &CEditorWindow::mousePick);

	connect(sceneUndoStack, &QUndoStack::cleanChanged, this, [=]() {
		updateTitle();

		if (gWorld->GetScene())
			gWorld->GetScene()->MarkAsDirty(!sceneUndoStack->isClean());
	});

	RestoreState();

	SetTool("Object Tool");

	updateTitle();
}

void CEditorWindow::engineUpdate()
{
	if (activeTool)
		activeTool->Update();
}

void CEditorWindow::SetGizmoMode(EGizmoMode mode)
{
	gizmoMode = mode;
	emit onGizmoModeChanged();
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
	else
	{
		CEntity* ent = nullptr;

		TObjectPtr<CObject> obj = hit.hitProxy->GetOwner();
		if (auto comp = CastChecked<CSceneComponent>(obj); comp)
			ent = comp->GetEntity();

		if (ent)
			DoEntityContextMenu(ent, QCursor::pos());
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
		for (auto* m : sceneMenuBar->children())
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
			curMenu = new QMenu(split[0].c_str(), sceneMenuBar);
			curMenu->setObjectName(split[0].c_str());
			//curMenu->setTitle();
			sceneMenuBar->addMenu(curMenu);
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

void CEditorWindow::SaveScene()
{
	emit onSaveScene();
}

int CEditorWindow::ExecSaveMessageBox()
{
	QMessageBox msg;
	msg.setText("Do you want to save before closing?");
	FString name = "unsaved scene";
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
	out.setValue("sceneDockManager", sceneDockManager->saveState());
}

void CEditorWindow::UserRestoreState(QSettings& in)
{
	sceneDockManager->restoreState(in.value("sceneDockManager").toByteArray());
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
