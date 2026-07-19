
#include "EditorEngine.h"
#include "EditorWindow.h"
#include "EngineThread.h"
#include "Widgets/ViewportWidget.h"
#include "Window.h"
#include "Game/World.h"
#include "Game/Input/InputManager.h"
#include "Game/GameInstance.h"
#include "Game/Events.h"
#include "Assets/Scene.h"
#include "EditorConfig.h"
#include "System.h"
#include "EditorPlugins.h"
#include "AssetThumbnail.h"

#include <Util/KeyValue.h>

#include "Rendering/Renderer.h"
#include "Rendering/RenderScene.h"
#include "Rendering/GraphicsInterface.h"
#include "Rendering/DefaultRenderer.h"
#include "Rendering/DebugRenderer.h"

#include "ImGui/imgui.h"

//CModule& GetModule_ThoriumEditor2();
REGISTER_DEFAULT_MODULE(ThoriumEditorQt)

CEditorVar evBoundBoxColor("BoundingBoxColor", "Viewport", FVariant(FColor::yellow), true);
CEditorVar evBoundBoxActiveColor("BoundingBoxActiveColor", "Viewport", FVariant(FColor::orange), true);

void CEditorEngine::Init()
{
	InitMinimal();
	gIsEditor = true;
	gIsClient = true;

	CModuleManager::RegisterModule(&GetModule_ThoriumEditorQt());

	// Create default implementations for physics and audio.
	CreatePhysicsApi(CModuleManager::FindClass("CJoltPhysicsApi"));
	CreateAudioInterface(CModuleManager::FindClass("CSdlAudioInterface"));

	viewportCams[0] = new CCameraProxy();
	viewportCams[1] = new CCameraProxy();
	viewportCams[2] = new CCameraProxy();
	viewportCams[3] = new CCameraProxy();

	CWindow::Init();

	gGHI = GetGraphicsInterface();
	gGHI->Init();

	gRenderer = CreateObject<CDefaultRenderer>();
	gRenderer->MakeIndestructible();
	gRenderer->Init();

	InitEditorRender();

	gameWindow = nullptr;

	inputManager = CreateObject<CInputManager>();

	//viewportWidth = 640; viewportHeight = 480;
	//sceneFrameBuffer = gGHI->CreateFrameBuffer(640, 480, TEXTURE_FORMAT_RGBA8_UNORM);

	//InitImGui();
	/*ImGuiIO& io = ImGui::GetIO();
	FString dataPath = OSGetDataPath() + "/ThoriumEngine/EditorConfig/imgui.ini";
	dataPath.ReplaceAll('\\', '/');
	io.IniFilename = (const char*)malloc(dataPath.Size() + 1);
	memcpy((char*)io.IniFilename, dataPath.Data(), dataPath.Size() + 1);
	ImGui::LoadIniSettingsFromDisk(io.IniFilename);*/

	if (!gameInstance)
		SetGameInstance<CGameInstance>();

	Events::PostLevelChange.Bind(this, &CEditorEngine::OnLevelChange);

	LoadWorld();
	//LoadWorld("maps/Basic.thasset");
	deltaTime = 0.02;
}

int CEditorEngine::Run()
{
	/*gIsRunning = true;

	deltaTime = 0.02;
	while (gIsRunning)*/
	{
		UpdateEvents(EventExec_PreUpdate);

		if (inputManager)
			inputManager->ClearCache();
		CWindow::PollEvents();
		if (inputManager)
			inputManager->BuildInput();

		CAssetManager::Update();
		CObjectManager::Update();

		//gGHI->ImGuiBeginFrame();

		if (!nextSceneName.IsEmpty())
		{
			DoLoadWorld();

			if (bIsPlaying)
				gWorld->Start();
		}

		Events::OnUpdate.Invoke();

		if (!bPaused || bStepFrame)
		{
			gWorld->Update(FMath::Min(deltaTime, 0.25));
			bStepFrame = false;
		}

		Events::PostUpdate.Invoke();
		UpdateEvents(EventExec_PostUpdate);

		if (bWantsToExit)
			gIsRunning = false;

		if (!gEditorWindow)
			return 0;

		CAssetThumbnailManager::Update();

		Events::OnRender.Invoke();
		UpdateEvents(EventExec_PreRender);

		//CViewportWidget* viewport = gEditorWindow->worldViewport;
		gWorld->renderScene->SetScreenPercentage(cvRenderScreenPercentage.AsFloat());

		if (gEditorWindow->gameViewport && gEditorWindow->gameViewport->GetSwapChain())
			gWorld->renderScene->SetFrameBuffer(gEditorWindow->gameViewport->GetSwapChain()->GetFrameBuffer());

		for (int i = 0; i < 4; i++)
		{
			if (gEditorWindow->worldViewports[i] && gEditorWindow->worldViewports[i]->GetSwapChain())
				viewportCams[i]->renderTarget = gEditorWindow->worldViewports[i]->GetSwapChain()->GetFrameBuffer();
		}

		if (!bGameView && !bIsPlaying && bSelectionBoundingBox)
		{
			for (auto& obj : selectedObjects)
			{
				CEntity* ent = Cast<CEntity>(obj);
				if (!ent)
					continue;

				FColor boxColor = evBoundBoxColor.GetValue().AsColor();
				FColor activeColor = evBoundBoxActiveColor.GetValue().AsColor();

				gWorld->renderScene->DebugRenderer()->DrawBounds(ent->GetBounds(), obj == activeObject ? activeColor : boxColor);
			}
		}

		gWorld->Render();
		gRenderer->PushScene(gWorld->GetRenderScene());

		gRenderer->Render();

		if (!bGameView)
			DoEditorRender();

		//viewport->GetSwapChain()->GetDepthBuffer()->Clear();
		//gGHI->SetFrameBuffer(viewport->GetSwapChain()->GetFrameBuffer(), viewport->GetSwapChain()->GetDepthBuffer());
		//gGHI->ImGuiRender();

		Events::PostRender.Invoke();
		UpdateEvents(EventExec_PostRender);

		for (int i = 0; i < 4; i++)
		{
			auto* viewport = gEditorWindow->worldViewports[i];
			if (viewport && viewport->GetSwapChain())
				viewport->GetSwapChain()->Present(gEditorWindow->activeViewport == viewport ? 1 : 0, 0);
		}

		if (gEditorWindow->gameViewport && gEditorWindow->gameViewport->GetSwapChain())
			gEditorWindow->gameViewport->GetSwapChain()->Present(0, 0);
	}

	return 0;
}

void CEditorEngine::OnExit()
{
	CEditorVar::Save();
	CEditorPlugins::Exit();

	CEngine::OnExit();
}

void CEditorEngine::PushEvent(IEditorEvent* event)
{
	eventMutex.lock();
	events.Add(event);
	eventMutex.unlock();
}

void CEditorEngine::PushEvent(EEventExec time, std::function<void()> func)
{
	class LambdaEvent : public IEditorEvent
	{
	public:
		LambdaEvent(std::function<void()> func) : IEditorEvent(), func(func)
		{
		}
		void Exec() override
		{
			func();
		}
	public:
		std::function<void()> func;
	};
	LambdaEvent* event = new LambdaEvent(func);
	event->execTime = time;
	PushEvent(event);
}

void CEditorEngine::SelectObject(CObject* obj)
{
	selectedObjects.Clear();
	activeObject = obj;
	if (obj)
		selectedObjects.Add(obj);

	emit gEngineThread->onSelectionChanged();
}

void CEditorEngine::SelectObjects(const TArray<CObject*>& objs)
{
	selectedObjects.Clear();
	activeObject = nullptr;

	for (auto* obj : objs)
		if (obj)
			selectedObjects.Add(obj);

	if (selectedObjects.Size() > 0)
		activeObject = *selectedObjects.last();

	emit gEngineThread->onSelectionChanged();
}

void CEditorEngine::AddSelectedObject(CObject* obj)
{
	selectedObjects.Add(obj);
	activeObject = obj;

	emit gEngineThread->onSelectionChanged();
}

void CEditorEngine::RemoveSelectedObject(CObject* obj)
{
	if (auto it = selectedObjects.Find(obj); it != selectedObjects.end())
		selectedObjects.Erase(it);

	if (activeObject == obj)
	{
		if (selectedObjects.Size() > 0)
			activeObject = *selectedObjects.last();
		else
			activeObject = nullptr;
	}

	emit gEngineThread->onSelectionChanged();
}

bool CEditorEngine::IsObjectSelected(CObject* obj)
{
	if (auto it = selectedObjects.Find(obj); it != selectedObjects.end())
		return true;
	return false;
}

void CEditorEngine::ClearSelection()
{
	selectedObjects.Clear();
	activeObject = nullptr;

	emit gEngineThread->onSelectionChanged();
}

void CEditorEngine::BakeLighting()
{
	if (!gWorld->GetScene())
		return;

	FString cmd = SSystem::GetEnginePath() + "/bin/win64/LightBaker.exe -scene \"" + gWorld->GetScene()->File()->Path() + "\"";
	if (bProjectLoaded)
		cmd += " -project \"" + activeGame.mod->Path() + "/../\"";

	int r = SSystem::Execute(cmd);
	QThread::msleep(100); // wait for the process to release the file lock on the light data.
	if (r == 0)
		gWorld->LoadLightData();
}

void CEditorEngine::OnLevelChange()
{
	if (!bIsPlaying)
	{
		gWorld->RegisterCamera(viewportCams[0]);
		gWorld->RegisterCamera(viewportCams[1]);
		gWorld->RegisterCamera(viewportCams[2]);
		gWorld->RegisterCamera(viewportCams[3]);
		gWorld->SetPrimaryCamera(viewportCams[0]);

		if (gWorld->GetScene() && gWorld->GetScene()->File())
		{
			//QString appdataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "\\ThoriumEngine";
			//QSettings settings(appdataPath + "\\EditorConfig\\" + Name + ".cfg", QSettings::Format::IniFormat);
			
			FString dataPath = SSystem::GetDataPath() + "/ThoriumEngine/EditorConfig/AssetUserData/";
			CScene* scene = gWorld->GetScene();

			FKeyValue kv(dataPath + "/" + FString::ToString(scene->AssetId()) + ".cfg");
			if (kv.IsOpen())
			{
				// Get camera data
				for (int i = 0; i < 4; i++)
				{
					KVCategory* c = kv.GetCategory("viewport_" + FString::ToString(i));
					if (!c)
						continue;

					int view = c->GetValue("view")->AsInt();
					FVector camPos = FVariant::FromString(*c->GetValue("position")).AsVector();
					FQuaternion camRot = FVariant::FromString(*c->GetValue("rotation")).AsQuat();

					viewportCams[i]->position = camPos;
					viewportCams[i]->rotation = camRot;

					gEditorWindow->worldViewports[i]->SetViewMode((ECameraView)view);
				}
			}
		}

		activeObject = nullptr;
		selectedObjects.Clear();
	}

	emit gEngineThread->onLevelChanged();
}

void CEditorEngine::OnSaveScene()
{
	gWorld->Save();

	FString dataPath = SSystem::GetDataPath() + "/ThoriumEngine/EditorConfig/AssetUserData/";
	CScene* scene = gWorld->GetScene();

	CFileSystem::OSCreateDirectory(dataPath);

	FKeyValue kv(dataPath + FString::ToString(scene->AssetId()) + ".cfg");
	for (int i = 0; i < 4; i++)
	{
		if (!viewportCams[i] || !gEditorWindow->worldViewports[i])
			continue;

		KVCategory* c = kv.GetCategory("viewport_" + FString::ToString(i), true);

		int view = gEditorWindow->worldViewports[i]->GetViewMode();
		c->SetValue("view", FVariant(view).ToString());
		c->SetValue("position", FVariant(viewportCams[i]->position).ToString());
		c->SetValue("rotation", FVariant(viewportCams[i]->rotation).ToString());
	}

	kv.Save();
}

void CEditorEngine::UpdateEvents(EEventExec time)
{
	eventMutex.lock();

	static TArray<IEditorEvent*> eventsToFree;
	eventsToFree.Clear();

	auto c = events;

	// unlock the mutex while executing events to prevent deadlocks if an event tries to push another event.
	eventMutex.unlock();

	for (auto* event : c)
	{
		if (event->execTime == time)
		{
			event->Exec();

			eventsToFree.Add(event);
		}
	}

	eventMutex.lock();

	for (auto* e : eventsToFree)
	{
		events.Erase(events.Find(e));
		delete e;
	}

	eventMutex.unlock();
}

void CEditorEngine::SetDeltaTime(double dt)
{
	deltaTime = dt;
}
