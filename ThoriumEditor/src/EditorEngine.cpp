
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

#include "Rendering/Renderer.h"
#include "Rendering/RenderScene.h"
#include "Rendering/GraphicsInterface.h"
#include "Rendering/DefaultRenderer.h"
#include "Rendering/DebugRenderer.h"

#include "ImGui/imgui.h"

//CModule& GetModule_ThoriumEditor2();
REGISTER_DEFAULT_MODULE(ThoriumEditorQt)

CEditorVar evBoundBoxColor("BoundingBoxColor", "Viewport", FVariant(FColor::yellow));
CEditorVar evBoundBoxActiveColor("BoundingBoxActiveColor", "Viewport", FVariant(FColor::orange));

void CEditorEngine::Init()
{
	InitMinimal();
	gIsEditor = true;
	gIsClient = true;

	CModuleManager::RegisterModule(&GetModule_ThoriumEditorQt());

	viewportCams[0] = new CCameraProxy();
	viewportCams[1] = new CCameraProxy();
	viewportCams[2] = new CCameraProxy();
	viewportCams[3] = new CCameraProxy();

	viewportCams[1]->bOrthographic = true;
	//viewportCams[1]->bDrawWireframe = true;
	viewportCams[1]->fov = 2;
	viewportCams[1]->position = FVector(0, 0, -1100);
	//viewportCams[1]->nearPlane = 100;
	viewportCams[1]->farPlane = 2000;

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

	CEditorVar::Load();

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
			if (gEditorWindow->worldViewports[i] && gEditorWindow->worldViewports[i]->GetSwapChain())
				gEditorWindow->worldViewports[i]->GetSwapChain()->Present(0, 0);
		}

		if (gEditorWindow->gameViewport && gEditorWindow->gameViewport->GetSwapChain())
			gEditorWindow->gameViewport->GetSwapChain()->Present(1, 0);
	}

	return 0;
}

void CEditorEngine::OnExit()
{
	CEditorVar::Save();

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
			CFStream sdkStream = gWorld->GetScene()->File()->GetSdkStream(".meta", "rb");
			if (sdkStream.IsOpen())
			{
				FVector camPos;
				FQuaternion camRot;

				sdkStream >> &camPos >> &camRot;

				viewportCams[0]->position = camPos;
				viewportCams[0]->rotation = camRot;
				sdkStream.Close();

				// reset the camController's camera so it uses the new oriantation.
				//camController->SetCamera(editorCamera);
			}
		}
	}

	emit gEngineThread->onLevelChanged();
}

void CEditorEngine::UpdateEvents(EEventExec time)
{
	eventMutex.lock();

	static TArray<IEditorEvent*> eventsToFree;
	eventsToFree.Clear();

	for (auto* event : events)
	{
		if (event->execTime == time)
		{
			event->Exec();

			eventsToFree.Add(event);
		}
	}

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
