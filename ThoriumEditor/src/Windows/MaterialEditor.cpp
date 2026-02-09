
#include "MaterialEditor.h"
#include "EditorEngine.h"
#include "Rendering/RenderScene.h"

#include "Game/Events.h"

#include "Widgets/ContentBrowser.h"
#include "Widgets/ViewportWidget.h"
#include "Widgets/FileDialogs.h"
#include "DockManager.h"
#include <QUndoStack>
#include <QBoxLayout>
#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QScrollArea>
#include <QLabel>
#include <QMenuBar>

SDK_REGISTER_WINDOW(CMaterialEditor, "Material Editor", "Tools", NULL);

class FMaterialOpenAction : public FAssetBrowserAction
{
public:
	FMaterialOpenAction()
	{
		type = BA_FILE_OPEN;
		targetClass = (FAssetClass*)CMaterial::StaticClass();
	}

	void Invoke(FBrowserActionData* d) override
	{
		auto* wnd = CToolsWindow::Create<CMaterialEditor>();
		FString path = d->file->Path();
		
		//gEditorEngine->PushEvent(EventExec_PreUpdate, [wnd, path]() {
			wnd->SetMaterial(CAssetManager::GetAsset<CMaterial>(path));
		//});
	}
} static FMaterialOpenAction_instance;

CMaterialEditor::~CMaterialEditor()
{
	Events::OnRender.RemoveAll(this);
	Events::PostRender.RemoveAll(this);

	world->Delete();
	world = nullptr;
}

void CMaterialEditor::SetupUi()
{
	dockmanager = new ads::CDockManager(this);
	dockmanager->setStyleSheet("");

	setWindowTitle("Material Editor");

	undoStack = new QUndoStack(this);

	menuFile = new QMenu("File"); menuBar->addMenu(menuFile);
	menuEdit = new QMenu("Edit"); menuBar->addMenu(menuEdit);

	menuFile->addAction("New", this, &CMaterialEditor::NewMaterial);
	menuFile->addAction("Open", this, &CMaterialEditor::OpenMaterial);
	menuFile->addAction("Save");
	menuFile->addAction("Save As");
	menuFile->addSeparator();
	menuFile->addAction("Close", this, [this]() { this->close(); });

	QAction* undo = undoStack->createUndoAction(this);
	QAction* redo = undoStack->createRedoAction(this);
	undo->setIcon(QIcon(":/icons/undo.svg"));
	redo->setIcon(QIcon(":/icons/redo.svg"));
	undo->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
	redo->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y));
	menuEdit->addAction(undo);
	menuEdit->addAction(redo);

	// Viewport
	{
		QWidget* widget = new QWidget(this);
		widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		QVBoxLayout* layout = new QVBoxLayout(this);
		widget->setLayout(layout);

		auto* sceneDock = new ads::CDockWidget("Viewport", this);
		sceneDock->setObjectName("materialeditor_viewport_dockwidget");
		sceneDock->setWidget(widget);
		dockmanager->addDockWidget(ads::CenterDockWidgetArea, sceneDock);
		sceneDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

		viewport = new CViewportWidget(this);
		layout->addWidget(viewport);
	}

	// Properties
	{
		//QScrollArea* propScroll = new QScrollArea(this);
		propertiesWidget = new QWidget(this);
		propertiesLayout = new QVBoxLayout(propertiesWidget);
		propertiesWidget->setLayout(propertiesLayout);
		//propScroll->setWidget(propertiesWidget);

		QLabel* infoLabel = new QLabel("No material selected", propertiesWidget);
		propertiesLayout->addWidget(infoLabel);

		propertiesDock = new ads::CDockWidget("Properties", this);
		propertiesDock->setObjectName("materialeditor_properties_dockwidget");
		propertiesDock->setWidget(propertiesWidget);
		dockmanager->addDockWidget(ads::LeftDockWidgetArea, propertiesDock);
		propertiesDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	}

	// Shader Settins
	{
		settingsWidget = new QWidget(this);
		settingsLayout = new QVBoxLayout(settingsWidget);

		settingsDock = new ads::CDockWidget("Shader Settings", this);
		settingsDock->setObjectName("materialeditor_shadersettings_dockwidget");

		settingsDock->setWidget(settingsWidget);
		dockmanager->addDockWidget(ads::CenterDockWidgetArea, settingsDock, propertiesDock->dockAreaWidget());
		settingsDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	}

	propertiesDock->setAsCurrentTab();

	RestoreState();

	//Init();
	gEditorEngine->PushEvent(EventExec_PreUpdate, [this]() { this->Init(); });
}

void CMaterialEditor::Init()
{
	world = CreateObject<CWorld>();
	world->InitWorld(CWorld::InitializeInfo().CreateAISystems(false).CreatePhyiscsWorld(false).RegisterForRendering(false));
	
	TObjectPtr<CEntity> modelEnt = world->CreateEntity<CEntity>();
	modelComp = modelEnt->AddComponent<CModelComponent>("Model");
	modelComp->SetModel("models/Sphere.thasset");

	cam = new CCameraProxy();
	viewport->SetControlMode(ECameraControlMode::Orbit);
	viewport->SetCamera(cam);

	world->RegisterCamera(cam);
	world->SetPrimaryCamera(cam);

	Events::OnRender.Bind(this, &CMaterialEditor::DoRender);
	Events::PostRender.Bind(this, &CMaterialEditor::SwapBuffers);
}

void CMaterialEditor::DoRender()
{
	if (!viewport->GetSwapChain())
		return;

	world->Update(gEngine->DeltaTime());
	world->Render();

	world->GetRenderScene()->SetFrameBuffer(viewport->GetSwapChain()->GetFrameBuffer());
	gRenderer->PushScene(world->GetRenderScene());
}

void CMaterialEditor::SwapBuffers()
{
	if (viewport->GetSwapChain())
		viewport->GetSwapChain()->Present(0, 0);
}

bool CMaterialEditor::Shutdown()
{
	if (!undoStack->isClean())
		return false;

	SaveState();
	return true;
}

void CMaterialEditor::SetMaterial(CMaterial* mat)
{
	if (material == mat)
		return;

	if (!undoStack->isClean())
	{
		return; // save material.
	}

	material = mat;
	//modelComp->SetMaterial(mat);
	UpdateProperties();

	gEditorEngine->PushEvent(EventExec_PostUpdate, [=]() { modelComp->SetMaterial(mat); });
}

void CMaterialEditor::SetMaterial(const FString& path)
{
}

void CMaterialEditor::NewMaterial()
{
	if (material && !undoStack->isClean())
	{
		// save material.
		return;
	}

	material = CreateObject<CMaterial>();
	material->SetShader("Simple");
}

void CMaterialEditor::OpenMaterial()
{
	COpenFileDialog dialog((FAssetClass*)CMaterial::StaticClass(), this);
	if (dialog.exec() && dialog.File())
	{
		auto m = CAssetManager::GetAsset<CMaterial>(dialog.File()->Path());
		SetMaterial(m);
	}
}

void CMaterialEditor::UpdateProperties()
{
	for (auto w : curProperties)
	{
		propertiesLayout->removeWidget((QWidget*)w);
		w->deleteLater();
	}
	curProperties.Clear();

	if (!material)
		return;

	//auto* shader = material->GetShaderSource();
	for (auto& prop : material->properties)
	{
		QLabel* item = new QLabel(prop.name.c_str());

		propertiesLayout->addWidget(item);
		curProperties.Add(item);
	}
}
