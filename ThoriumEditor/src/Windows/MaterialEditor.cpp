
#include "MaterialEditor.h"
#include "EditorEngine.h"
#include "Rendering/RenderScene.h"

#include "Game/Events.h"

#include "Widgets/ContentBrowser.h"
#include "Widgets/ViewportWidget.h"
#include "DockManager.h"
#include <QUndoStack>
#include <QBoxLayout>
#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QScrollArea>
#include <QLabel>

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
		
		//wnd->SetMaterial(CAssetManager::GetAsset<CMaterial>(d->file->Path()));
		gEditorEngine()->PushEvent(EventExec_PreUpdate, [wnd, path]() {
			wnd->SetMaterial(CAssetManager::GetAsset<CMaterial>(path));
		});
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
	CToolsWindow::SetupUi();

	dockmanager = new ads::CDockManager(this);
	dockmanager->setStyleSheet("");

	setWindowTitle("Material Editor");

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

		viewport = new CViewportWidget(this);
		layout->addWidget(viewport);
	}

	// Properties
	{
		//QScrollArea* propScroll = new QScrollArea(this);
		propertiesWidget = new QWidget(this);
		QVBoxLayout* layout = new QVBoxLayout(propertiesWidget);
		propertiesWidget->setLayout(layout);
		//propScroll->setWidget(propertiesWidget);

		QLabel* infoLabel = new QLabel("No material selected", propertiesWidget);
		layout->addWidget(infoLabel);

		propertiesDock = new ads::CDockWidget("Properties", this);
		propertiesDock->setObjectName("materialeditor_properties_dockwidget");
		propertiesDock->setWidget(propertiesWidget);
		dockmanager->addDockWidget(ads::LeftDockWidgetArea, propertiesDock);
	}

	// Shader Settins
	{
		//settingsDock = new ads::CDockWidget("Shader Settings", this);
		//settingsDock->setObjectName("materialeditor_shadersettings_dockwidget");
		//settingsDock->setWidget(settingsView);
		//dockmanager->addDockWidget(ads::LeftDockWidgetArea, settingsDock);
	}

	RestoreState();

	//Init();
	gEditorEngine()->PushEvent(EventExec_PreUpdate, [this]() { this->Init(); });
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
	SaveState();
	return true;
}

void CMaterialEditor::SetMaterial(CMaterial* mat)
{
	if (material == mat)
		return;

	material = mat;
	modelComp->SetMaterial(mat);
	//UpdateProperties();
}

void CMaterialEditor::NewMaterial()
{
}

void CMaterialEditor::OpenMaterial()
{
}

void CMaterialEditor::UpdateProperties()
{
	//for (auto w : curProperties)
	//{
	//	propertiesWidget->layout()->removeWidget((QWidget*)w);
	//	w->deleteLater();
	//}
	curProperties.Clear();

	if (!material)
		return;

	//auto* shader = material->GetShaderSource();
	
	for (auto& prop : material->properties)
	{
		QLabel* item = new QLabel(prop.name.c_str(), propertiesWidget);

		propertiesWidget->layout()->addWidget(item);
		curProperties.Add(item);
	}
}
