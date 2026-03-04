
#include "MaterialEditor.h"
#include "EditorEngine.h"
#include "Rendering/RenderScene.h"

#include "Game/Events.h"

#include "Widgets/PropertyEditors/ArrayProperty.h"
#include "Widgets/PropertyEditors/IntProperty.h"
#include "Widgets/PropertyEditors/FloatProperty.h"
#include "Widgets/PropertyEditors/BoolProperty.h"
#include "Widgets/PropertyEditors/EnumProperty.h"
#include "Widgets/PropertyEditors/StringProperty.h"
#include "Widgets/PropertyEditors/StructProperty.h"
#include "Widgets/PropertyEditors/ObjectPtrProperty.h"
#include "Widgets/PropertyEditors/VectorProperty.h"

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
#include <QThread>
#include <QComboBox>
#include <QCheckBox>
#include <QMessageBox>

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

	material = nullptr;
	modelComp = nullptr;
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
	menuFile->addAction("Save", this, &CMaterialEditor::SaveMaterial);
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
		propertiesLayout->setSpacing(0);
		//propScroll->setWidget(propertiesWidget);

		//QLabel* infoLabel = new QLabel("No material selected", propertiesWidget);
		//propertiesLayout->addWidget(infoLabel);

		shaderEdit = new CObjectPtrProperty("Shader", nullptr, CShaderSource::StaticClass(), this);
		shaderEdit->AllowNull(false);
		shaderEdit->setMaximumHeight(42);
		propertiesLayout->addWidget(shaderEdit);

		connect(shaderEdit, &CObjectPtrProperty::OnValueChanged, this, [=]() { gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() { material->Validate(); }); UpdateProperties(); });

		propertiesDock = new ads::CDockWidget("Properties", this);
		propertiesDock->setObjectName("materialeditor_properties_dockwidget");
		propertiesDock->setWidget(propertiesWidget);
		dockmanager->addDockWidget(ads::LeftDockWidgetArea, propertiesDock);
		propertiesDock->setFeature(ads::CDockWidget::DockWidgetClosable, false);

		QScrollArea* scrollArea = new QScrollArea(this);
		scrollArea->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
		scrollArea->setWidgetResizable(true);
		scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		propertiesLayout->addWidget(scrollArea);

		QWidget* scrollWidget = new QWidget(this);
		contentLayout = new QVBoxLayout(scrollWidget);
		scrollWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
		contentLayout->setContentsMargins(0, 0, 0, 0);
		contentLayout->setSpacing(0);
		scrollArea->setWidget(scrollWidget);
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

		QScrollArea* scrollArea = new QScrollArea(this);
		scrollArea->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
		scrollArea->setWidgetResizable(true);
		scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		settingsLayout->addWidget(scrollArea);

		QWidget* scrollWidget = new QWidget(this);
		auto* layout = new QVBoxLayout(scrollWidget);
		scrollWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
		//layout->setContentsMargins(0, 0, 0, 0);
		//layout->setSpacing(0);
		scrollArea->setWidget(scrollWidget);

		renderPassCombo = new QComboBox(this);
		renderPassCombo->addItem("Deffered");
		renderPassCombo->addItem("Opaque Deffered, Transparent Forward");
		renderPassCombo->addItem("Forward");

		layout->addWidget(CreateEditorLayout("Render Pass", renderPassCombo));

		forceTransparentEdit = new QCheckBox(this);
		layout->addWidget(CreateEditorLayout("Force Transparent Pass", forceTransparentEdit));

		receiveShadowsEdit = new QCheckBox(this);
		layout->addWidget(CreateEditorLayout("Receive Shadows", receiveShadowsEdit));

		castShadowsEdit = new QCheckBox(this);
		layout->addWidget(CreateEditorLayout("Cast Shadows", castShadowsEdit));

		depthTestEdit = new QCheckBox(this);
		layout->addWidget(CreateEditorLayout("Depth Test", depthTestEdit));

		connect(renderPassCombo, &QComboBox::currentTextChanged, this, [=]() { gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() { material->preferredRenderPass = renderPassCombo->currentIndex(); }); });
		connect(forceTransparentEdit, &QCheckBox::toggled, this, [=]() { gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() { material->bForceTransparentPass = forceTransparentEdit->isChecked(); }); });
		connect(receiveShadowsEdit, &QCheckBox::toggled, this, [=]() { gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() { material->bReceiveShadows = receiveShadowsEdit->isChecked(); }); });
		connect(castShadowsEdit, &QCheckBox::toggled, this, [=]() { gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() { material->bCastShadows = castShadowsEdit->isChecked(); }); });
		connect(depthTestEdit, &QCheckBox::toggled, this, [=]() { gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() { material->bDepthTest = depthTestEdit->isChecked(); }); });
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

QWidget* CMaterialEditor::CreateEditorLayout(const QString& name, QWidget* edit)
{
	QLabel* lbl = new QLabel(name, this);

	QWidget* container = new QWidget(this);
	auto* layout = new QHBoxLayout();
	container->setLayout(layout);

	layout->addWidget(lbl);
	layout->addStretch();
	layout->addWidget(edit);
	return container;
}

bool CMaterialEditor::Shutdown()
{
	if (!undoStack->isClean())
	{
		if (!TrySaveMaterial())
			return false;
	}

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
		if (!TrySaveMaterial());
			return;
	}

	undoStack->clear();

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

void CMaterialEditor::SaveMaterial()
{
	if (!material)
		return;

	if (!material->File())
	{
		CSaveFileDialog dialog(this);
		if (!dialog.exec())
			return;

		if (!CAssetManager::RegisterNewAsset(material, dialog.Path(), dialog.Mod()))
			return;
	}

	material->Save();
	undoStack->setClean();
}

bool CMaterialEditor::TrySaveMaterial()
{
	if (!undoStack->isClean())
	{
		int r = ExecSavePopup();
		if (r == QMessageBox::Cancel)
			return false;

		if (r == QMessageBox::Save)
			SaveMaterial();

		if (r == QMessageBox::Discard)
			RevertChanges();
	}
	return true;
}

int CMaterialEditor::ExecSavePopup()
{
	QMessageBox msg;
	msg.setText("Do you want to save before closing?");
	FString name = "new material";
	if (material->File())
		name = material->File()->Name();

	msg.setInformativeText(name.c_str());
	msg.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
	msg.setDefaultButton(QMessageBox::Save);
	return msg.exec();
}

void CMaterialEditor::RevertChanges()
{
	if (material->File())
		material->Init();

	undoStack->clear();
}

void CMaterialEditor::UpdateProperties()
{
	for (auto w : curProperties)
	{
		contentLayout->removeWidget((QWidget*)w);
		w->deleteLater();
	}
	curProperties.Clear();
	shaderEdit->SetValue(nullptr);

	QThread::msleep(10); // wait for material to be updated in game thread.

	if (!material)
		return;

	shaderEdit->SetValue(&material->shader);

	renderPassCombo->setCurrentIndex(material->preferredRenderPass);
	forceTransparentEdit->setChecked(material->bForceTransparentPass);
	receiveShadowsEdit->setChecked(material->bReceiveShadows);
	castShadowsEdit->setChecked(material->bCastShadows);
	depthTestEdit->setChecked(material->bDepthTest);

	//auto* shader = material->GetShaderSource();
	for (auto& prop : material->properties)
	{
		QLabel* item = new QLabel(prop.name.c_str());

		contentLayout->addWidget(item);
		curProperties.Add(item);
	}
}
