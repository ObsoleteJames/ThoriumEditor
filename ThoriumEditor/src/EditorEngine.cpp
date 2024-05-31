
#include <string>

#include "EditorEngine.h"
#include "Module.h"
#include "Console.h"
#include "Window.h"
#include "Layer.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderScene.h"
#include "Rendering/DebugRenderer.h"
#include "Rendering/Texture.h"
#include "Game/World.h"
#include "Game/Events.h"
#include "Game/GameInstance.h"
#include "Game/Input/InputManager.h"
#include "Game/Components/CameraComponent.h"
#include "Game/Components/ModelComponent.h"
#include "Game/Entities/ModelEntity.h"
#include "Resources/Material.h"
#include "Resources/Scene.h"
#include "FileDialogs.h"
#include "Misc/Timer.h"
#include <Util/KeyValue.h>

#include "EditorMenu.h"
#include "AssetBrowserWidget.h"
#include "ClassSelectorPopup.h"
#include "Debug/ObjectDebugger.h"
#include "Layers/PropertyEditor.h"
#include "Layers/ConsoleWidget.h"
#include "Layers/InputOutputWidget.h"
#include "Layers/ProjectSettings.h"
#include "Layers/MaterialEditor.h"
#include "Layers/AddonsWindow.h"
#include "Layers/ModelEditor.h"
#include "Layers/EditorSettings.h"
#include "Layers/EditorLog.h"
#include "Layers/ProjectManager.h"

#include <map>

#include "Platform/Windows/DirectX/DirectXRenderer.h"
#include "Platform/Windows/DirectX/DirectXFrameBuffer.h"
#include "Platform/Windows/DirectX/DirectXTexture.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/ImGui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"

#define IMGUIZMO_API SDK_API
#include "ImGuizmo.h"

#include "ThemeManager.h"

#define TEX_VIEW(tex) ((DirectXTexture2D*)tex)->view

FEditorLog gBuildLog("Build");

FEditorShortcut scSaveScene("Save Scene", "Editor", ImGuiKey_S, 0, 1);
FEditorShortcut scSaveSceneAs("Save Scene As", "Editor", ImGuiKey_S, 1, 1);
FEditorShortcut scNewScene("New Scene", "Editor", ImGuiKey_N, 0, 1);
FEditorShortcut scOpenScene("Open Scene", "Editor", ImGuiKey_O, 0, 1);

FEditorShortcut scNewProject("New Project", "Project", ImGuiKey_N, 1, 1);
FEditorShortcut scOpenProject("Open Project", "Project", ImGuiKey_O, 1, 1);

void CEditorEngine::Init()
{
	InitMinimal();
	gIsEditor = true;
	gIsClient = true;

	rootMenu = new CEditorMenu(FString());
	SetupMenu();

	CLayer::Init();

	assetBrowser = new CAssetBrowserWidget();
	//propertyEditor = AddLayer<CPropertyEditor>();
	//consoleWidget = AddLayer<CConsoleWidget>();
	//ioWidget = AddLayer<CInputOutputWidget>();
	//projSettingsWidget = AddLayer<CProjectSettingsWidget>();
	//projSettingsWidget->bEnabled = false;
	//addonsWindow = AddLayer<CAddonsWindow>();
	//addonsWindow->bEnabled = false;
	//editorSettings = AddLayer<CEditorSettingsWidget>();
	//editorSettings->bEnabled = false;

	//logWnd = AddLayer<CEditorLogWnd>();
	//logWnd->bEnabled = false;

	//objectDebuggerWidget = AddLayer<CObjectDebugger>();
	//objectDebuggerWidget->bEnabled = false;

	editorCamera = new CCameraProxy();
	editorCamera->position = { 0, 1, -1 };
	camController = new CCameraController();
	camController->SetCamera(editorCamera);

	LoadEditorConfig();

	CWindow::Init();

	Renderer::CreateRenderer<DirectXRenderer>();

	gameWindow = new CWindow(editorCfg.wndWidth, editorCfg.wndHeight, editorCfg.wndPosX, editorCfg.wndPosY, "Thorium Editor");
	gameWindow->SetSwapChain(gRenderer->CreateSwapChain(gameWindow));
	gameWindow->SetWindowMode((CWindow::EWindowMode)editorCfg.wndMode);

	gameWindow->OnKeyEvent.Bind(this, &CEditorEngine::KeyEventA);
	gameWindow->OnCursorMove.Bind(this, [=](double x, double y) {
		gameWindow->mouseX = x - viewportX;
		gameWindow->mouseY = y - viewportY;
	});

	gameWindow->SetIcon(engineMod->Path() + "/editor/thorium editor icon.png");

	inputManager = CreateObject<CInputManager>();
	inputManager->SetInputWindow(gameWindow);
	inputManager->SetShowCursor(true);

	//if (!inputManager)
	//{
	//	inputManager = CreateObject<CInputManager>();
	//	inputManager->LoadConfig();
	//}

	//inputManager->SetInputWindow(gameWindow);
	//inputManager->SetShowCursor(true);

	viewportWidth = 1280;
	viewportHeight = 720;
	sceneFrameBuffer = gRenderer->CreateFrameBuffer(1280, 720, TEXTURE_FORMAT_RGBA8_UNORM);
	//sceneDepthBuffer = gRenderer->CreateDepthBuffer({ 1280, 720, TH_DBF_D24_S8, 1, false });

	CFileSystem::OSCreateDirectory(OSGetDataPath() + "/ThoriumEngine/EditorConfig");

	InitImGui();
	ImGuiIO& io = ImGui::GetIO();
	FString dataPath = OSGetDataPath() + "/ThoriumEngine/EditorConfig/imgui.ini";
	dataPath.ReplaceAll('\\', '/');
	io.IniFilename = (const char*)malloc(dataPath.Size() + 1);
	memcpy((char*)io.IniFilename, dataPath.Data(), dataPath.Size() + 1);
	ImGui::LoadIniSettingsFromDisk(io.IniFilename);

	ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
	gizmoMode = ImGuizmo::TRANSLATE;

	ThoriumEditor::LoadThemes();
	ThoriumEditor::SetTheme("Default");

	if (!gameInstance)
		SetGameInstance<CGameInstance>();

	InitEditorData();

	Events::PostLevelChange.Bind(this, &CEditorEngine::OnLevelChange);

	//if (bProjectLoaded)
	//{
	//	assetBrowser->SetDir(activeGame.mod->Name(), FString());
	//	LoadWorld(ToFString(activeGame.startupScene));
	//}

	LoadWorld();
}

int CEditorEngine::Run()
{
	gIsRunning = true;

	deltaTime = 0.02;
	while (gIsRunning)
	{
		FTimer dtTimer;
		
		if (inputManager)
			inputManager->ClearCache();
		CWindow::PollEvents();
		if (inputManager)
			inputManager->BuildInput();

		CResourceManager::Update();
		CObjectManager::Update();

		gRenderer->ImGuiBeginFrame();
		ImGuizmo::BeginFrame();

		if (!nextSceneName.IsEmpty())
		{
			DoLoadWorld();

			if (bIsPlaying)
				gWorld->Start();
		}

		FTimer updateTimer;

		Events::OnUpdate.Invoke();
		if (!bPaused || bStepFrame)
		{
			gWorld->Update(FMath::Min(deltaTime, 0.25));
			bStepFrame = false;
		}

		Events::PostUpdate.Invoke();

		for (auto* l : removeLayers)
			RemoveLayer(l);
		removeLayers.Clear();

		for (auto& l : layers)
			if (l->bEnabled)
				l->OnUpdate(deltaTime);

		updateTimer.Stop();
		updateTime = updateTimer.GetMiliseconds();

		updateTimer.Begin();

#if RENDER_MULTITHREADED
		gRenderer->JoinRenderThread();
#endif

		// Update scene framebuffer
		{
			int w, h;
			sceneFrameBuffer->GetSize(w, h);
			if (w != viewportWidth || h != viewportHeight)
			{
				sceneFrameBuffer->Resize(viewportWidth, viewportHeight);
				//gWorld->renderScene->
				//sceneDepthBuffer->Resize(viewportWidth, viewportHeight);
			}
		}

		UpdateEditor();

		updateTimer.Stop();
		editorUpdateTime = updateTimer.GetMiliseconds();

		updateTimer.Begin();
		gRenderer->BeginRender();

		Events::OnRender.Invoke();

		gWorld->renderScene->SetScreenPercentage(cvRenderScreenPercentage.AsFloat());
		gWorld->renderScene->SetFrameBuffer(sceneFrameBuffer);
		//gWorld->renderScene->SetDepthBuffer(sceneDepthBuffer);

		if (!bIsPlaying)
		{
			DrawSelectionDebug();

			FDrawMeshCmd cmd;
			cmd.material = gridMat;
			cmd.mesh = &gridMesh;
			cmd.transform = FMatrix(1.f);
			cmd.drawType |= MESH_DRAW_PRIMITIVE_LINES;

			FRenderCommand gridDraw(cmd, R_DEBUG_PASS);
			gWorld->renderScene->PushCommand(gridDraw);
		}

		gWorld->Render();
		gRenderer->PushScene(gWorld->renderScene);

#if RENDER_MULTITHREADED
		gRenderer->RenderMT();
#else
		gRenderer->Render();
#endif
		updateTimer.Stop();
		renderTime = updateTimer.GetMiliseconds();

		updateTimer.Begin();

		gameWindow->swapChain->GetDepthBuffer()->Clear();
		gRenderer->SetFrameBuffer(gameWindow->swapChain->GetFrameBuffer(), gameWindow->swapChain->GetDepthBuffer());
		gRenderer->ImGuiRender();

		updateTimer.Stop();
		imguiRenderTime = updateTimer.GetMiliseconds();

		gameWindow->Present(userConfig.bVSync, 0);

		dtTimer.Stop();
		deltaTime = dtTimer.GetSeconds();

		if (gameWindow->WantsToClose() || bWantsToExit)
			gIsRunning = false;
	}

	gRenderer->ImGuiShutdown();

	OnExit();
	return 0;
}

void CEditorEngine::OnExit()
{
	CONSOLE_LogInfo("CEngine", "Shutting down...");

	SaveEditorConfig();

	ThoriumEditor::ClearThemeIcons();

	delete assetBrowser;
	gWorld->Delete();
	delete gRenderer;
	delete gameWindow;

	gPhysicsApi->Shutdown();
	gPhysicsApi->Delete();
	gPhysicsApi = nullptr;

	CWindow::Shutdown();

	CResourceManager::Shutdown();
	CModuleManager::Cleanup();

	SaveConsoleLog();
	CConsole::Shutdown();
}

bool CEditorEngine::LoadProject(const FString& path)
{
	bool r = CEngine::LoadProject(path);

	bOpenProj = !r;
	return r;
}

void CEditorEngine::LoadEditorConfig()
{
	FKeyValue kv(GetEditorConfigPath() + "/Editor.cfg");
	if (!kv.IsOpen())
	{
		CONSOLE_LogWarning("CEditorEngine", "Failed to load editor config, unable to find config file!");
		return;
	}

	editorCfg.wndPosX = kv.GetValue("wndPosX")->AsInt(100);
	editorCfg.wndPosY = kv.GetValue("wndPosY")->AsInt(100);
	editorCfg.wndWidth = kv.GetValue("wndWidth")->AsInt(1920);
	editorCfg.wndHeight = kv.GetValue("wndHeight")->AsInt(1080);
	editorCfg.wndMode = kv.GetValue("wndMode")->AsInt(1);

	menuViewOutliner->bChecked = kv.GetValue("view_outliner")->AsBool(true);
	menuAssetBrowser->bChecked = kv.GetValue("view_assetbrowser")->AsBool(true);
	menuStatistics->bChecked = kv.GetValue("view_statistics")->AsBool();
	
	editorCamera->fov = kv.GetValue("camera_fov")->AsFloat(90.f);
	camController->cameraSpeed = kv.GetValue("camera_speed")->AsInt(4);

	CLayer::LoadConfig(kv);

	KVCategory* projs = kv.GetCategory("projects", true);
	for (auto& v : projs->GetValues())
	{
		FProject proj;
		proj.name = v.Key;
		proj.dir = v.Value.Value;

		if (LoadProjectConfig(proj.dir, proj))
			RegisterProject(proj);
	}

	FEditorShortcut::LoadConfig();
}

void CEditorEngine::SaveEditorConfig()
{
	FKeyValue kv(GetEditorConfigPath() + "/Editor.cfg");

	gameWindow->UpdateWindowRect();
	gameWindow->GetSize(editorCfg.wndWidth, editorCfg.wndHeight);
	
	kv.SetValue("wndPosX", FString::ToString(gameWindow->WindowedRect.x));
	kv.SetValue("wndPosY", FString::ToString(gameWindow->WindowedRect.y));
	kv.SetValue("wndWidth", FString::ToString(editorCfg.wndWidth));
	kv.SetValue("wndHeight", FString::ToString(editorCfg.wndHeight));
	kv.SetValue("wndMode", FString::ToString((int)gameWindow->GetWindowMode()));

	kv.SetValue("view_outliner", FString::ToString((int)menuViewOutliner->bChecked));
	kv.SetValue("view_assetbrowser", FString::ToString((int)menuAssetBrowser->bChecked));
	kv.SetValue("view_statistics", FString::ToString((int)menuStatistics->bChecked));

	kv.SetValue("camera_fov", FString::ToString((int)editorCamera->fov));
	kv.SetValue("camera_speed", FString::ToString((int)camController->cameraSpeed));

	CLayer::SaveConfig(kv);

	KVCategory* projs = kv.GetCategory("projects", true);
	for (auto& p : availableProjects)
		projs->SetValue(p.name, p.dir);

	kv.Save();

	FEditorShortcut::SaveConfig();
}

void CEditorEngine::CompileProjectCode(int config)
{
	FString cmd = OSGetEnginePath(ENGINE_VERSION) + "/bin/win64/BuildTool.exe \"";
	cmd += CFileSystem::GetCurrentPath() + "/.project/" + activeGame.name + "/Build.cfg\" ";
#if PLATFORM_WINDOWS
	cmd += "-x64 ";
#endif

//#if _DEBUG
//	cmd += "-debug";
//#elif _DEVELOPMENT
//	cmd += "-development";
//#elif _RELEASE
//	cmd += "-release";
//#endif

	switch (config)
	{
	case 0:
		cmd += "-release";
		break;
	case 1:
		cmd += "-development";
		break;
	case 2:
		cmd += "-debug";
		break;
	}

	ExecuteProgram(cmd);
	ExecuteProgram("cmake -A x64 -B \"" + CFileSystem::GetCurrentPath() + "/.project/" + activeGame.name + "/Intermediate/Build\" \"" + CFileSystem::GetCurrentPath() + "/.project/" + activeGame.name + "/Intermediate\"");
	ExecuteProgram("cmake --build \"" + CFileSystem::GetCurrentPath() + "/.project/" + activeGame.name + "/Intermediate/Build\"");
}

void CEditorEngine::InitEditorData()
{
	TArray<FVertex> boxVerts = {
		{ { -1, 1, 1 }, {}, {}, {}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ { 1, 1, 1 }, {}, {}, {}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ { -1, 1, -1 }, {}, {}, {}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ { 1, 1, -1 }, {}, {}, {}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ {-1, -1, 1 }, {}, {}, {}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ { 1, -1, 1 }, {}, {}, {}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ { -1, -1, -1 }, {}, {}, {}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ { 1, -1, -1 }, {}, {}, {}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
	};

	TArray<uint> boxInds = {
		0, 1,
		0, 2,
		2, 3,
		1, 3,

		4, 5,
		4, 6,
		6, 7,
		5, 7,

		0, 4,
		1, 5,
		2, 6,
		3, 7
	};

	boxOutlineMesh.vertexBuffer = gRenderer->CreateVertexBuffer(boxVerts);
	boxOutlineMesh.indexBuffer = gRenderer->CreateIndexBuffer(boxInds);
	boxOutlineMesh.numVertices = (uint32)boxVerts.Size();
	boxOutlineMesh.numIndices = (uint32)boxInds.Size();

	boxOutlineMesh.topologyType = FMesh::TOPOLOGY_LINES;

	outlineMat = CreateObject<CMaterial>();
	outlineMat->SetShader("Tools");
	outlineMat->SetInt("vType", 4);
	outlineMat->SetColor("vColorTint", FColor(1.f, 0.88f, 0.4f));

	GenerateGrid(100, 1, &gridMesh);
	gridMat = CreateObject<CMaterial>();
	gridMat->SetShader("Tools");
	gridMat->SetInt("vType", 1);
}

void CEditorEngine::GenerateGrid(float gridSize, float quadSize, FMesh* outMesh)
{
	int numGrids = int(gridSize / quadSize);

	TArray<FVertex> verts;
	//TArray<uint> indices;

	for (int i = 0; i < numGrids + 1; i++)
	{
		float halfGrid = (float)(numGrids / 2);

		FVertex a{};
		FVertex b{};

		a.color = { 0.2f, 0.2f, 0.2f };
		b.color = { 0.2f, 0.2f, 0.2f };

		a.position.x = float(i) - halfGrid;
		a.position.z = -halfGrid;
		b.position.x = a.position.x;
		b.position.z = halfGrid;

		if (i == (int)halfGrid)
		{
			a.color = { 0.2f, 0.2f, 0.7f };
			b.color = { 0.2f, 0.2f, 0.7f };
		}

		verts.Add(a);
		verts.Add(b);
	}
	for (int i = 0; i < numGrids + 1; i++)
	{
		float halfGrid = (float)(numGrids / 2);

		FVertex a{};
		FVertex b{};

		a.color = { 0.2f, 0.2f, 0.2f };
		b.color = { 0.2f, 0.2f, 0.2f };

		a.position.z = float(i) - halfGrid;
		a.position.x = -halfGrid;
		b.position.z = a.position.z;
		b.position.x = halfGrid;

		if (i == (int)halfGrid)
		{
			a.color = { 0.7f, 0.2f, 0.2f };
			b.color = { 0.7f, 0.2f, 0.2f };
		}

		verts.Add(a);
		verts.Add(b);
	}

	//outMesh->numIndices = indices.Size();
	outMesh->numVertices = (uint32)verts.Size();

	//outMesh->indexBuffer = gRenderer->CreateIndexBuffer(indices);
	outMesh->vertexBuffer = gRenderer->CreateVertexBuffer(verts);
}

void CEditorEngine::NewScene()
{
	//if ()

	LoadWorld();
}

bool CEditorEngine::SaveScene()
{
	if (!gWorld->GetScene()->File())
	{
		ThoriumEditor::SaveFile("saveEditorScene", (FAssetClass*)CScene::StaticClass());
		return false;
	}

	gWorld->Save();

	CFStream sdkStream = gWorld->GetScene()->File()->GetSdkStream("wb");
	if (sdkStream.IsOpen())
	{
		FVector camPos = editorCamera->position;
		FQuaternion camRot = editorCamera->rotation;

		sdkStream << &camPos << &camRot;
		sdkStream.Close();
	}
	return true;
}

void CEditorEngine::StartPlay()
{
	if (!SaveScene())
		return;

	gWorld->Start();
	bIsPlaying = true;
	inputManager->EnableInput();
}

void CEditorEngine::StopPlay()
{
	gWorld->Stop();
	bIsPlaying = false;
	gWorld->SetPrimaryCamera(editorCamera);

	// Reload the scene
	CScene* scene = gWorld->GetScene();
	gWorld->Delete();

	gWorld = CreateObject<CWorld>();
	gWorld->MakeIndestructible();

	Events::LevelChange.Invoke();

	gWorld->LoadScene(scene);
	gWorld->InitWorld(CWorld::InitializeInfo().RegisterForRendering(false));

	Events::PostLevelChange.Invoke();
}

void CEditorEngine::DoMousePick()
{
	if (!gWorld)
		return;

	if (ImGuizmo::IsOver())
		return;

	if (selectMode != ESelectMode_Object && selectedEntities.Size() != 0)
		return;

	FRay ray = FRay::MouseToRay(editorCamera, InputManager()->GetMousePos() - FVector2(viewportX, viewportY), { (float)viewportWidth, (float)viewportHeight });
	ray.direction = ray.direction.Normalize();

	auto* scene = gWorld->GetRenderScene();

	FPrimitiveHitInfo hit;

	if (scene->RayCast(ray.origin, ray.direction, &hit))
	{
		CEntity* ent = nullptr;

		TObjectPtr<CObject> obj = hit.hitProxy->GetOwner();
		if (auto comp = CastChecked<CSceneComponent>(obj); comp)
		{
			ent = comp->GetEntity();
		}

		//gDebugRenderer->DrawLine(ray.origin, hit.position, FColor::green, 3, true);
		//gDebugRenderer->DrawLine(ray.origin, ray.origin + ray.direction * 2.f, FColor::red, 3, true);
		//gDebugRenderer->DrawLine(hit.position, hit.position + hit.normal, FColor::navy_blue, 6, true);

		//gDebugRenderer->DrawBox(FTransform(hit.position, FQuaternion(), FVector(0.02f)), FColor::cyan.WithAlpha(0.3f), DebugDrawType_Solid | DebugDrawType_Overlay, 3);

		if (ent)
		{
			if (ImGui::IsKeyDown(ImGuiKey_ModCtrl))
			{
				if (!IsEntitySelected(ent))
					AddSelectedEntity(ent);
				else
					RemoveSelectedEntity(ent);
			}
			else
				SetSelectedEntity(ent);
		}
	}
	else
		SetSelectedEntity(nullptr);
}

void CEditorEngine::DoMaterialDrop(TObjectPtr<CMaterial> mat, bool bPeek)
{
	if (bPeek)
	{
		//static FPrimitiveHitInfo prevHit{};
		//static CMaterial* prevMat = nullptr;

		//FPrimitiveHitInfo hit;
		//auto* scene = gWorld->GetRenderScene();
		//FRay ray = FRay::MouseToRay(editorCamera, InputManager()->GetMousePos() - FVector2(viewportX, viewportY), { (float)viewportWidth, (float)viewportHeight });
		//ray.direction = ray.direction.Normalize();

		//if (scene->RayCast(ray.origin, ray.direction, &hit))
		//{
		//	if (prevHit.hitProxy && (prevHit.hitProxy != hit.hitProxy || prevHit.materialIndex != hit.materialIndex))
		//	{
		//		TObjectPtr<CObject> obj = prevHit.hitProxy->GetOwner();
		//		if (auto comp = CastChecked<CModelComponent>(obj); comp)
		//		{
		//			comp->SetMaterial(prevMat, hit.materialIndex);
		//		}
		//	}

		//	TObjectPtr<CObject> obj = hit.hitProxy->GetOwner();
		//	if (auto comp = CastChecked<CModelComponent>(obj); comp)
		//	{
		//		if (prevHit.hitProxy != hit.hitProxy || prevHit.materialIndex != hit.materialIndex)
		//			prevMat = comp->GetMaterial(hit.materialIndex);

		//		comp->SetMaterial(mat, hit.materialIndex);
		//	}

		//	prevHit = hit;
		//}
		//else if (prevHit.hitProxy)
		//{
		//	TObjectPtr<CObject> obj = prevHit.hitProxy->GetOwner();
		//	if (auto comp = CastChecked<CModelComponent>(obj); comp)
		//	{
		//		comp->SetMaterial(prevMat, prevHit.materialIndex);
		//	}
		//}
	}
	else
	{
		FPrimitiveHitInfo hit;
		auto* scene = gWorld->GetRenderScene();
		FRay ray = FRay::MouseToRay(editorCamera, InputManager()->GetMousePos() - FVector2(viewportX, viewportY), { (float)viewportWidth, (float)viewportHeight });
		ray.direction = ray.direction.Normalize();

		if (scene->RayCast(ray.origin, ray.direction, &hit))
		{
			TObjectPtr<CObject> obj = hit.hitProxy->GetOwner();
			if (auto comp = CastChecked<CModelComponent>(obj); comp)
			{
				comp->SetMaterial(mat, hit.materialIndex);
			}
		}
	}
}

void CEditorEngine::DoModelAssetDrop(TObjectPtr<CModelAsset> mdl, bool bPeek)
{
	if (bPeek)
	{
		

	}
	else
	{
		FPrimitiveHitInfo hit;
		auto* scene = gWorld->GetRenderScene();
		FRay ray = FRay::MouseToRay(editorCamera, InputManager()->GetMousePos() - FVector2(viewportX, viewportY), { (float)viewportWidth, (float)viewportHeight });
		ray.direction = ray.direction.Normalize();

		if (!scene->RayCast(ray.origin, ray.direction, &hit))
			hit.position = ray.origin + ray.direction;

		CModelEntity* mdlEnt = gWorld->CreateEntity<CModelEntity>(mdl->File()->Name() + " Entity");
		mdlEnt->SetModel(mdl);
		mdlEnt->SetPosition(hit.position - FVector(0, mdl->GetBounds().Min().y, 0));
	}
}

void CEditorEngine::OnLevelChange()
{
	selectedEntities.Clear();
	selectedObject = nullptr;

	if (!bIsPlaying)
	{
		gWorld->SetPrimaryCamera(editorCamera);

		if (gWorld->GetScene() && gWorld->GetScene()->File())
		{
			CFStream sdkStream = gWorld->GetScene()->File()->GetSdkStream("rb");
			if (sdkStream.IsOpen())
			{
				FVector camPos;
				FQuaternion camRot;

				sdkStream >> &camPos >> &camRot;

				editorCamera->position = camPos;
				editorCamera->rotation = camRot;
				sdkStream.Close();

				// reset the camController's camera so it uses the new oriantation.
				camController->SetCamera(editorCamera);
			}
		}
	}
}

void CEditorEngine::ToggleGameInput()
{
	static bool bShowedCursor = inputManager->CursorVisible();
	if (inputManager->InputEnabled())
		bShowedCursor = inputManager->CursorVisible();
	inputManager->SetInputEnabled(!inputManager->InputEnabled());
	inputManager->SetShowCursor(inputManager->InputEnabled() ? bShowedCursor : true);
}

void CEditorEngine::DrawSelectionDebug()
{
	if (selectedEntities.Size() == 0)
		return;

	for (auto obj : selectedEntities)
	{
		FBounds b = obj->GetBounds();

		if (b.Size().Magnitude() == 0)
			continue;

		FDrawMeshCmd cmd;
		cmd.material = outlineMat;
		cmd.mesh = &boxOutlineMesh;
		cmd.transform = FMatrix(1.f).Translate(b.position).Scale(b.extents);
		cmd.drawType |= MESH_DRAW_PRIMITIVE_LINES;

		FRenderCommand gridDraw(cmd, R_DEBUG_PASS);
		gWorld->renderScene->PushCommand(gridDraw);
	}
}

void CEditorEngine::FocusOnSelection()
{
	if (selectedEntities.Size() == 0)
		return;

	FBounds b;

	for (auto obj : selectedEntities)
		b = b.Combine(obj->GetBounds());

	editorCamera->position = b.position - editorCamera->GetForwardVector() * FMath::Max(b.extents.Magnitude() * 1.5f, 1.f);
}

void CEditorEngine::KeyEventA(EKeyCode key, EInputAction action, EInputMod mod)
{
	if (key == EKeyCode::F1 && mod == IM_SHIFT && action == IE_RELEASE)
	{
		ToggleGameInput();
	}
}

void CEditorEngine::SaveProjectConfig()
{
	FKeyValue kv(projectConfig.dir + "/config/project.cfg");

	kv.SetValue("name", projectConfig.name);
	kv.SetValue("displayName", projectConfig.displayName);
	kv.SetValue("author", projectConfig.author);
	kv.SetValue("game", projectConfig.game);
	kv.SetValue("hasSdk", projectConfig.bIncludesSdk ? "true" : "false");
	kv.SetValue("hasEngineContent", projectConfig.bHasEngineContent ? "true" : "false");
	
	auto* addons = kv.GetArray("addons", true);
	*addons = projectConfig.addons;

	kv.Save();

	FKeyValue phys(projectConfig.dir + "/config/physics.cfg");
	phys.SetValue("api", physicsSettings.api.Get() ? physicsSettings.api.Get()->cppName : "CJoltPhysicsApi");
	phys.Save();

	FKeyValue kvGame(projectConfig.game + "/config/gameinfo.cfg");

	kvGame.SetValue("title", activeGame.title);
	kvGame.SetValue("version", activeGame.version);
	kvGame.SetValue("scene", activeGame.startupScene);
	kvGame.SetValue("gameinstance", activeGame.gameInstanceClass.Get() ? activeGame.gameInstanceClass.Get()->cppName : "CGameInstance");
	kvGame.SetValue("inputmanager", activeGame.inputManagerClass.Get() ? activeGame.inputManagerClass.Get()->cppName : "CInputManager");

	kvGame.Save();
}

void CEditorEngine::RegisterMenu(CEditorMenu* menu, const FString& path /*= FString()*/)
{
	CEditorMenu* parent = rootMenu;
	if (!path.IsEmpty())
		parent = GetMenu(path);

	parent->children.Add(menu);
	menu->parent = parent;

	parent->SortChildren();
}

CEditorMenu* CEditorEngine::GetMenu(const FString& path)
{
	TArray<FString> paths = path.Split("/\\");

	CEditorMenu* curMenu = rootMenu;
	for (auto& p : paths)
	{
		bool bFound = false;
		for (auto* m : curMenu->children)
		{
			if (m->Name() == p)
			{
				curMenu = m;
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			CEditorMenu* m = new CEditorMenu(p, FString());
			curMenu->children.Add(m);
			m->parent = curMenu;
			curMenu = m;
		}
	}

	if (curMenu == rootMenu)
		return nullptr;

	return curMenu;
}

void CEditorEngine::SetSelectMode(ESelectMode mode)
{
	selectMode = mode;
	selectedBone = -1; 
	boneComponent = nullptr;
}

void CEditorEngine::RegisterProject(const FProject& proj)
{
	for (auto& p : availableProjects)
		if (p.name == proj.name || p.dir == proj.dir)
			return;

	availableProjects.Add(proj);
}

bool CEditorEngine::IsEntitySelected(CEntity* ent)
{
	auto it = selectedEntities.Find(ent);
	if (it != selectedEntities.end())
		return true;
	return false;
}

void CEditorEngine::AddSelectedEntity(CEntity* ent)
{
	auto it = selectedEntities.Find(ent);
	if (it != selectedEntities.end())
		return;

	selectedEntities.Add(ent);
	selectedObject = ent;
}

void CEditorEngine::RemoveSelectedEntity(CEntity* ent)
{
	auto it = selectedEntities.Find(ent);
	if (it != selectedEntities.end())
		selectedEntities.Erase(it);

	if (selectedObject == ent && selectedEntities.Size() > 0)
		selectedObject = (CObject*)*selectedEntities.last();
}

void CEditorEngine::SetSelectedEntity(CEntity* ent)
{
	selectedEntities.Clear();
	if (ent)
		selectedEntities.Add(ent);
	selectedObject = ent;
}

void CEditorEngine::AddLayer(TObjectPtr<CLayer> layer)
{
	layers.Add(layer);
	layer->OnAttach();
}

void CEditorEngine::RemoveLayer(TObjectPtr<CLayer> layer)
{
	auto it = layers.Find(layer);
	if (it != layers.end())
	{
		layers.Erase(it);
		layer->OnDetach();
	}
}

void CEditorEngine::PollRemoveLayer(TObjectPtr<CLayer> layer)
{
	removeLayers.Add(layer);
}

void CEditorEngine::CopyEntity()
{
	if (selectedEntities.Size() == 0)
		return;

	copyBuffer.dataType = CB_ENTITY;
	copyBuffer.data = FMemStream();
	copyBuffer.type = selectedEntities[0]->GetClass();

	selectedEntities[0]->Serialize(copyBuffer.data);
}

void CEditorEngine::PasteEntity(const FVector& pos)
{
	if (copyBuffer.dataType == CB_NONE)
		return;

	CEntity* ent = gWorld->CreateEntity(copyBuffer.type, FString());

	copyBuffer.data.Seek(SEEK_SET, 0);
	ent->Load(copyBuffer.data);
	ent->SetWorldPosition(pos);

	SetSelectedEntity(ent);
}

void CEditorEngine::SceneFileDialogs()
{
	FString f;
	FString m;
	if (ThoriumEditor::AcceptFile("saveEditorScene", &f, &m) && !f.IsEmpty())
	{
		CResourceManager::RegisterNewResource(gWorld->GetScene(), f, m);
		gWorld->Save();
	}
	if (ThoriumEditor::AcceptFile("openEditorScene", &f, &m) && !f.IsEmpty())
	{
		LoadWorld(f);
	}
}

void CEditorEngine::DupeEntity()
{

}

void CEditorEngine::DrawMenu(CEditorMenu* m)
{
	if (m->children.Size() > 0)
	{
		if (ImGui::BeginMenu(m->Name().c_str()))
		{
			for (auto& c : m->children)
				DrawMenu(c);

			ImGui::EndMenu();
		}
	}
	else
	{
		if (ImGui::MenuItem(m->Name().c_str(), m->Shortcut().c_str(), m->bToggle ? m->bEnabled : false))
		{
			if (m->OnClicked)
				m->OnClicked();

			if (m->bToggle)
			{
				m->bEnabled ^= 1;

				if (m->bEnabled && m->OnEnabled)
					m->OnEnabled();
				else if (m->OnDisabled)
					m->OnDisabled();
			}
		}
	}
}

void CEditorEngine::UpdateGizmos()
{
	if (selectMode == ESelectMode_Object)
		UpdateGizmoEntity();
	else if (selectMode == ESelectMode_Skeleton)
		UpdateGizmoSkeleton();
}

void CEditorEngine::UpdateGizmoEntity()
{
	if (selectedEntities.Size() == 0)
		return;

	float delta[16];

	FVector centerPos;
	for (int i = 0; i < (bGizmoLocal ? 1 : (int)selectedEntities.Size()); i++)
		centerPos += selectedEntities[i]->RootComponent()->GetPosition();

	centerPos /= (float)selectedEntities.Size();

	bool bUseLocal = bGizmoLocal && selectedEntities.Size() == 1;

	if (!ImGuizmo::IsUsing())
	{
		manipulationMatrix = FMatrix(1.f).Translate(centerPos);
		if (bUseLocal)
		{
			manipulationMatrix *= selectedEntities[0]->RootComponent()->GetRotation();
			manipulationMatrix = manipulationMatrix.Scale(selectedEntities[0]->RootComponent()->GetScale());
		}
	}

	int wndX, wndY;
	gameWindow->GetWindowPos(&wndX, &wndY);

	ImGuizmo::SetRect(viewportX + (float)wndX, viewportY + (float)wndY, viewportWidth, viewportHeight);
	bool r = ImGuizmo::Manipulate(editorCamera->view.v, editorCamera->projection.v, (ImGuizmo::OPERATION)gizmoMode, bUseLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD, manipulationMatrix.v, delta);

	FTransform deltaTranform;
	float _temp[3];
	(*(FMatrix*)delta).Decompose(deltaTranform.position, deltaTranform.scale, deltaTranform.rotation);
	ImGuizmo::DecomposeMatrixToComponents(delta, &deltaTranform.position.x, _temp, &deltaTranform.scale.x);

	if (r)
	{
		for (int i = 0; i < selectedEntities.Size(); i++)
		{
			CSceneComponent* root = selectedEntities[i]->RootComponent();
			if ((gizmoMode & ImGuizmo::TRANSLATE) != 0)
				root->SetPosition(root->GetPosition() + deltaTranform.position);
			if ((gizmoMode & ImGuizmo::ROTATE) != 0)
			{
				if (bUseLocal)
					root->SetRotation(root->GetRotation() * deltaTranform.rotation);
				else
					root->SetRotation(deltaTranform.rotation * root->GetRotation());
			}
			if ((gizmoMode & ImGuizmo::SCALE) != 0)
				root->SetScale(root->GetScale() * deltaTranform.scale);
		}
	}
}

void CEditorEngine::UpdateGizmoSkeleton()
{
	DrawSelectedSkeleton();

	if (selectedBone != -1 && boneComponent)
	{
		auto& skel = *(FSkeletonInstance*)&boneComponent->GetSkeleton();

		CModelAsset* model = boneComponent->GetModel();
		if (!model)
			return;

		int i = selectedBone;
		const FBone& bone = model->GetSkeleton().bones[i];
		const FBone* parent = bone.parent != -1 ? &model->GetSkeleton().bones[bone.parent] : nullptr;

		FTransform boneTransform = boneComponent->GetBoneModelTransform(i) * boneComponent->GetWorldTransform();

		if (!ImGuizmo::IsUsing())
		{
			manipulationMatrix = FMatrix(1.f).Translate(boneTransform.position);
			//if (bGizmoLocal)
				manipulationMatrix *= boneTransform.rotation;
		}

		int wndX, wndY;
		gameWindow->GetWindowPos(&wndX, &wndY);

		float delta[16];

		ImGuizmo::SetRect(viewportX + (float)wndX, viewportY + (float)wndY, viewportWidth, viewportHeight);
		bool r = ImGuizmo::Manipulate(editorCamera->view.v, editorCamera->projection.v, (ImGuizmo::OPERATION)gizmoMode, bGizmoLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD, manipulationMatrix.v, delta);

		FTransform deltaTranform;
		float _temp[3];
		(*(FMatrix*)delta).Decompose(deltaTranform.position, deltaTranform.scale, deltaTranform.rotation);
		ImGuizmo::DecomposeMatrixToComponents(delta, &deltaTranform.position.x, _temp, &deltaTranform.scale.x);

		if (r)
		{
			if (gizmoMode == ImGuizmo::TRANSLATE)
				skel.bones[i].position += boneTransform.rotation.Invert().Rotate(deltaTranform.position);
			if (gizmoMode == ImGuizmo::ROTATE)
			{
				if (bGizmoLocal)
					skel.bones[i].rotation *= deltaTranform.rotation;
				else
					skel.bones[i].rotation = (deltaTranform.rotation * (skel.bones[i].rotation * boneComponent->GetWorldRotation())) * boneComponent->GetWorldRotation().Invert();
			}

			boneComponent->UpdateSkeletonMatrix();
		}
	}
}

void CEditorEngine::DrawSelectedSkeleton()
{
	for (auto& ent : selectedEntities)
	{
		for (auto& comp : ent->GetAllComponents())
		{
			auto mdlComp = CastChecked<CModelComponent>(comp.second);
			if (mdlComp == nullptr)
				continue;

			CModelAsset* model = mdlComp->GetModel();
			if (!model)
				continue;

			auto& skel = mdlComp->GetSkeleton();

			int hovered = -1;

			for (int i = 0; i < (int)model->GetSkeleton().bones.Size(); i++)
			{
				const FBone& bone = model->GetSkeleton().bones[i];
				const FBone* parent = bone.parent != -1 ? &model->GetSkeleton().bones[bone.parent] : nullptr;

				FTransform boneTransform = mdlComp->GetBoneModelTransform(i) * mdlComp->GetWorldTransform();

				/*FVector bonePos = skel.bones[i].position + bone.position;
				if (parent)
					bonePos = skel.bones[bone.parent].rotation.Rotate(bonePos) + skel.bones[bone.parent].position;
				bonePos = mdlComp->GetWorldRotation().Rotate(bonePos * mdlComp->GetWorldScale()) + mdlComp->GetWorldPosition();

				FQuaternion boneRot = bone.rotation * skel.bones[i].rotation;
				if (parent)
					boneRot *= skel.bones[bone.parent].rotation;
				boneRot *= mdlComp->GetWorldRotation();*/

				bool bHover = (selectedBone == i && mdlComp == boneComponent);
				if (!bHover && hovered == -1 && !ImGuizmo::IsOver())
				{
					FRay ray = FRay::MouseToRay(editorCamera, InputManager()->GetMousePos() - FVector2(viewportX, viewportY), { (float)viewportWidth, (float)viewportHeight });
					ray.direction = ray.direction.Normalize();
					bHover = FMath::RayBox(FBounds(boneTransform.position, FVector(0.05f)), boneTransform.rotation, ray);
					if (bHover)
						hovered = i;
				}

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && bHover)
				{
					boneComponent = mdlComp;
					selectedBone = i;
				}

				gDebugRenderer->DrawBox(FTransform(boneTransform.position, boneTransform.rotation, FVector(0.1f)), bHover ? FColor::lime : FColor::blue.WithAlpha(0.5f), DebugDrawType_Solid | DebugDrawType_Overlay);
			}
		}
	}
}

FEditorShortcut::FEditorShortcut(const FString& n, const FString& c, ImGuiKey k, bool shift /*= false*/, bool ctrl /*= false*/) : name(n), context(c), key(k), bShift(shift), bCtrl(ctrl)
{
	GetShortcuts().Add(this);
	_SetString();

	friendlyName = n;
	friendlyName.ReplaceAll(' ', '_');
}

FEditorShortcut::operator bool()
{
	if ((bShift == ImGui::IsKeyDown(ImGuiKey_LeftShift)) && (bCtrl == ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) && ImGui::IsKeyPressed(key))
		return true;
	return false;
}

void FEditorShortcut::SetKey(ImGuiKey k, bool shift, bool ctrl)
{
	if (k == 0 || (k > ImGuiKey_Tab && k < ImGuiKey_GamepadStart))
	{
		key = k;
		bShift = shift;
		bCtrl = ctrl;
		_SetString();
	}
}

static const char* ImGuiKeyNames[] = {
	"None"
	"Tab",
	"Left Arrow",
	"Right Arrow",
	"Up Arrow",
	"Down Arrow",
	"Page Up",
	"Page Down",
	"Home",
	"End",
	"Insert",
	"Delete",
	"Backspace",
	"Space",
	"Enter",
	"Escape",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	"Menu",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"Apostrophe",        // '
	"Comma",             // ,
	"Minus",             // -
	"Period",            // .
	"Slash",             // /
	"Semicolon",         // ;
	"Equal",             // =
	"Left Bracket",      // [
	"Backslash",         // \ (this text inhibit multiline comment caused by backslash)
	"Right Bracket",     // ]
	"Grave Accent",
	"Caps Lock",
	"Scroll Lock",
	"NumLock",
	"Print Screen",
	"Pause",
	"Keypad 0", "Keypad 1", "Keypad 2", "Keypad 3", "Keypad 4",
	"Keypad 5", "Keypad 6", "Keypad 7", "Keypad 8", "Keypad 9",
	"Keypad Decimal",
	"Keypad Divide",
	"Keypad Multiply",
	"Keypad Subtract",
	"Keypad Add",
	"Keypad Enter",
	"Keypad Equal",
};

static constexpr SizeType ImGuiKeyNamesCount = IM_ARRAYSIZE(ImGuiKeyNames);

void FEditorShortcut::_SetString()
{
	if (key == ImGuiKey_None)
	{
		asString = "None";
		return;
	}

	const char* txt = ImGuiKeyNames[(int)key - (int)ImGuiKey_Tab];
	if (!txt)
	{
		key = ImGuiKey_None;
		asString = "None";
		return;
	}

	asString.Clear();

	if (bCtrl && bShift)
		asString = "Ctrl+Shift+";
	else if (bCtrl && !bShift)
		asString = "Ctrl+";
	else if (!bCtrl && bShift)
		asString = "Shift+";

	asString += txt;
}

void FEditorShortcut::SaveConfig()
{
	FKeyValue kv(CEditorEngine::GetEditorConfigPath() + "/Shortcuts.cfg");

	for (auto& sc : GetShortcuts())
		kv.SetValue(sc->context + "." + sc->friendlyName, sc->ToString());
	
	kv.Save();
}

void FEditorShortcut::LoadConfig()
{
	FKeyValue kv(CEditorEngine::GetEditorConfigPath() + "/Shortcuts.cfg");
	if (!kv.IsOpen())
		return;

	for (auto& v : kv.GetValues())
	{
		FString context;
		FString name;

		context = v.Key;
		name = v.Key;

		SizeType dotI = context.FindFirstOf('.');
		if (dotI == -1)
			continue;

		context.Erase(context.begin() + dotI, context.end());
		name.Erase(name.begin(), name.begin() + dotI + 1);

		for (auto* sc : GetShortcuts())
		{
			if (sc->friendlyName == name && sc->context == context)
			{
				ImGuiKey key = ImGuiKey_None;
				bool shift = false;
				bool ctrl = false;

				FString keyName = v.Value;
				if (keyName.Find("Ctrl") != -1)
					ctrl = true;
				if (keyName.Find("Shift") != -1)
					shift = true;
				if (auto i = keyName.FindLastOf('+'); i != -1)
					keyName.Erase(keyName.begin(), keyName.begin() + i + 1);

				for (int i = 0; i < ImGuiKeyNamesCount; i++)
				{
					if (ImGuiKeyNames[i] == nullptr)
						continue;

					if (keyName == ImGuiKeyNames[i])
					{
						key = (ImGuiKey)(i + ImGuiKey_Tab);
						break;
					}
				}

				if (key != ImGuiKey_None)
					sc->SetKey(key, shift, ctrl);

				break;
			}
		}
	}
}

TArray<FEditorShortcut*>& FEditorShortcut::GetShortcuts()
{
	static TArray<FEditorShortcut*> shortcuts;
	return shortcuts;
}

//TArray<FEditorShortcut*> FEditorShortcut::shortcuts;
