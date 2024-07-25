
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
#include "Assets/Material.h"
#include "Assets/Scene.h"
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

enum EMenuAction {
	MenuAction_NONE,
	MenuAction_NewScene,
	MenuAction_OpenScene,
	MenuAction_SaveScene,
	MenuAction_SaveSceneAs,
	MenuAction_OpenProject,
	MenuAction_NewProject
};
int menuAction = 0;

void CEditorEngine::UpdateEditor()
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", nullptr, window_flags);
	ImGui::PopStyleVar(3);

	ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
	static bool bInitDock = false;
	if (!bInitDock)
	{
		bInitDock = true;
		SetupEditorDocking();
	}

	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	SceneFileDialogs();

	if (ImGui::BeginMenuBar())
	{
		rootMenu->Render();

		ImGui::EndMenuBar();
	}

	ImGui::End();

	if (menuImGuiDemo->bChecked)
		ImGui::ShowDemoWindow(&menuImGuiDemo->bChecked);

	menuCloseProject->SetEnabled(bProjectLoaded);

	if (!bIsPlaying)
	{
		if (menuAction == MenuAction_SaveScene || (scSaveScene && bViewportHasFocus))
			SaveScene();
		if (menuAction == MenuAction_NewScene || (scNewScene && bViewportHasFocus))
			NewScene();
		if (menuAction == MenuAction_OpenScene)
			ThoriumEditor::OpenFile("openEditorScene", (FAssetClass*)CScene::StaticClass());

		if (menuAction == MenuAction_OpenProject || scOpenProject)
			CProjectManager::Open(0);
		if (menuAction == MenuAction_NewProject || scNewProject)
			CProjectManager::Open(1);
	}

	if (menuAction != 0)
		menuAction = 0;

	if (bOpenProj && !CProjectManager::IsOpen())
	{
		CProjectManager::Open(0);
		bOpenProj = false;
	}

	ImGui::SetNextWindowSize(ImVec2(785, 510), ImGuiCond_FirstUseEver);

	for (auto& l : layers)
		if (l->bEnabled)
			l->OnUIRender();

	// Viewport
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	if (ImGui::Begin("Scene##_gameSceneViewport"))
	{
		auto wndSize = ImGui::GetContentRegionAvail();
		auto cursorPos = ImGui::GetCursorScreenPos();

		ImGuizmo::SetDrawlist();

		bViewportHasFocus = ImGui::IsWindowFocused();

		ImGui::PopStyleVar(2);

		if (ImGui::BeginChild("sceneToolBar", ImVec2(wndSize.x, 32)))
		{
			ITexture2D* btnSave = ThoriumEditor::GetThemeIcon("floppy");
			ImGui::SetCursorPos({ 3.f, 3.f });

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			if (ImGui::ImageButton("##btnSaveScene", TEX_VIEW(btnSave), ImVec2(16, 16)))
				SaveScene();
			ImGui::PopStyleColor();

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			ImGui::Button("Create Object", ImVec2(0, 0));

			if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
			{
				DrawObjectCreateMenu();
				ImGui::EndPopup();
			}

			FClass* createEnt = nullptr;
			if (ThoriumEditor::AcceptClass("SceneCreateEntity", &createEnt) && createEnt)
			{
				auto* scene = gWorld->GetRenderScene();
				FVector entPos = scene->GetPrimaryCamera() ? scene->GetPrimaryCamera()->position + scene->GetPrimaryCamera()->rotation.Rotate(FVector(0, 0, 1)) : FVector();

				auto* ent = gWorld->CreateEntity(createEnt, createEnt->GetName());
				ent->SetPosition(entPos);
			}

			ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine(); ImGui::Text("Select Mode:");
			//ImGui::SameLine(); if (ImGui::Button("Object##selectMode")) SetSelectMode(ESelectMode_Object);
			//ImGui::SameLine(); if (ImGui::Button("Skeleton##selectMode")) SetSelectMode(ESelectMode_Skeleton);
			int selm = SelectMode();
			const char* selectModeNames[] = { "Object", "Skeleton", "Vertices", "Faces", "Edges" };
			ImGui::SameLine(); if (ImGui::TypeSelector("##", &selm, 5, selectModeNames, ImVec2(300, 24))) SetSelectMode((ESelectMode)selm);

			//ImGui::SetCursorScreenPos(cursorPos + ImVec2(wndSize.x / 2 - 100, 4));
			ITexture2D* btnPlay = ThoriumEditor::GetThemeIcon("btn-play");
			ITexture2D* btnPause = ThoriumEditor::GetThemeIcon("btn-pause");
			ITexture2D* btnStop = ThoriumEditor::GetThemeIcon("btn-stop");
			ITexture2D* btnStep = ThoriumEditor::GetThemeIcon("btn-stepframe");

			ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();
			if (ImGui::ImageButtonClear("Play/Pause", (bIsPlaying && !bPaused) ? TEX_VIEW(btnPause) : TEX_VIEW(btnPlay), ImVec2(10, 10)))
			{
				if (!bIsPlaying)
				{
					StartPlay();
					bPaused = false;
				}
				else
					bPaused ^= 1;
			}
			ImGui::SameLine();
			if (ImGui::ImageButtonClear("Stop", TEX_VIEW(btnStop), ImVec2(10, 10)) && bIsPlaying)
			{
				StopPlay();
				bPaused = false;
			}
			//bPaused ^= 1;
			ImGui::SameLine();
			if (ImGui::ImageButtonClear("Step Frame", TEX_VIEW(btnStep), ImVec2(10, 10)))
				bStepFrame = true;
		}
		ImGui::EndChild();

		int wndX, wndY;
		gameWindow->GetWindowPos(&wndX, &wndY);

		wndSize = ImGui::GetContentRegionAvail();
		cursorPos = ImGui::GetCursorScreenPos();
		viewportX = cursorPos.x - (float)wndX;
		viewportY = cursorPos.y - (float)wndY;

		DirectXFrameBuffer* fb = (DirectXFrameBuffer*)sceneFrameBuffer;
		ImGui::Image(fb->view, { wndSize.x, wndSize.y });

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* content = ImGui::AcceptDragDropPayload("THORIUM_ASSET_FILE");
			const ImGuiPayload* peek = ImGui::AcceptDragDropPayload("THORIUM_ASSET_FILE", ImGuiDragDropFlags_AcceptPeekOnly);
			if (content)
			{
				FFile* file = *(FFile**)content->Data;
				FAssetClass* type = CAssetManager::GetAssetTypeByFile(file);
				if (type == (FAssetClass*)CScene::StaticClass())
				{
					LoadWorld(file->Path());
				}
				if (type == (FAssetClass*)CMaterial::StaticClass())
				{
					CMaterial* mat = CAssetManager::GetAsset<CMaterial>(file->Path());
					DoMaterialDrop(mat, false);
				}
				if (type == (FAssetClass*)CModelAsset::StaticClass())
				{
					CModelAsset* mdl = CAssetManager::GetAsset<CModelAsset>(file->Path());
					DoModelAssetDrop(mdl, false);
				}
			}
			if (peek)
			{
				FFile* file = *(FFile**)peek->Data;
				FAssetClass* type = CAssetManager::GetAssetTypeByFile(file);
				if (type == (FAssetClass*)CMaterial::StaticClass())
				{
					CMaterial* mat = CAssetManager::GetAsset<CMaterial>(file->Path());
					DoMaterialDrop(mat, true);
				}
			}
			ImGui::EndDragDropTarget();
		}

		camController->Update(deltaTime);
		DoEntRightClick();

		if (!ImGui::IsMouseDown(ImGuiMouseButton_Right) && bViewportHasFocus)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_W))
				gizmoMode = ImGuizmo::TRANSLATE;
			if (ImGui::IsKeyPressed(ImGuiKey_E))
				gizmoMode = ImGuizmo::SCALE;
			if (ImGui::IsKeyPressed(ImGuiKey_R))
				gizmoMode = ImGuizmo::ROTATE;
		}

		if (ImGui::IsItemClicked() && inputManager && !inputManager->InputEnabled() && bIsPlaying && !bPaused)
			ToggleGameInput();

		if (ImGui::IsItemClicked() && !bIsPlaying)
			DoMousePick();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.75f, 0.75f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 0.3f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.25f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);

		ImGui::SetCursorScreenPos(cursorPos + ImVec2(8, 8));

		ImGui::SetNextItemWidth(24);
		ImGui::Button("=##_buttonCameraSettings");

		if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
		{
			ImGui::PopStyleVar();
			ImGui::DragFloat("FOV", &editorCamera->fov, 1.f, 25, 160);
			ImGui::DragFloat("Near Clip", &editorCamera->nearPlane, 0.1f, 0.001f, 10000.f);
			ImGui::DragFloat("Far Clip", &editorCamera->farPlane, 0.1f, 0.001f, 10000.f);
			ImGui::DragInt("Camera Speed", &camController->cameraSpeed, 0.1f, camController->minCamSpeed, camController->maxCamSpeed);

			float screenPercentage = cvRenderScreenPercentage.AsFloat();
			if (ImGui::DragFloat("Screen Percentage", &screenPercentage))
			{
				int w = gWorld->renderScene->GetFrameBufferWidth();
				int h = gWorld->renderScene->GetFrameBufferHeight();
				gWorld->renderScene->ResizeBuffers(w, h);
			}
			cvRenderScreenPercentage.SetValue(screenPercentage);

			ImGui::Checkbox("VSync", &userConfig.bVSync);

			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
			ImGui::EndPopup();
		}

		ImGui::SetNextItemWidth(24);
		ImGui::SameLine();
		ImGui::Button("View##_renderSettings");

		if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
		{
			ImGui::PopStyleVar();
			if (ImGui::MenuItem("Lit"))
				CConsole::Exec("r.materialmode 0");
			if (ImGui::MenuItem("Unlit"))
				CConsole::Exec("r.materialmode 1");
			if (ImGui::MenuItem("Normal"))
				CConsole::Exec("r.materialmode 2");

			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
			ImGui::EndPopup();
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor(4);

		FString windowPosTxt = "Viewport Pos: " + FString::ToString((int)viewportX) + "x" + FString::ToString((int)viewportY);
		auto mp = InputManager()->GetMousePos();
		FString mousePosTxt = "Mouse Pos: " + FString::ToString((int)mp.x) + "x" + FString::ToString((int)mp.y);

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.6, 0.1, 1));
		ImGui::RenderText(cursorPos + ImGui::GetStyle().FramePadding + ImVec2(0, 24), windowPosTxt.c_str());
		ImGui::RenderText(cursorPos + ImGui::GetStyle().FramePadding + ImVec2(0, 38), mousePosTxt.c_str());
		ImGui::PopStyleColor();

		viewportWidth = FMath::Max((int)wndSize.x, 32);
		viewportHeight = FMath::Max((int)wndSize.y, 32);
	}
	else
		ImGui::PopStyleVar(2);
	ImGui::End();

	DoEntityShortcuts();

	// Scene outliner
	if (menuViewOutliner->bChecked)
		DrawOutliner();

	// Asset Browser
	if (menuAssetBrowser->bChecked)
	{
		if (ImGui::Begin("Asset Browser##_editorAssetBrowser", &menuAssetBrowser->bChecked))
		{
			assetBrowser->RenderUI();
		}
		ImGui::End();
	}

	// Statistics
	if (menuStatistics->bChecked)
	{
		if (ImGui::Begin("Statistics##_editorStats", &menuStatistics->bChecked))
		{
			static float frameTimeSlow = 0.2f;
			static float frameTimeAccum = 0;
			static int _accum = 0;

			frameTimeAccum += deltaTime;
			_accum++;

			if (_accum > 16)
			{
				frameTimeSlow = frameTimeAccum / 16.f;
				frameTimeAccum = 0;
				_accum = 0;
			}

			// Time
			ImGui::Text("FPS: %.1f", 1.f / frameTimeSlow);
			ImGui::Text("frame time: %.2f(ms)", frameTimeSlow * 1000.f);
			ImGui::Text("update: %.2f(ms)", updateTime);
			ImGui::Text("render: %.2f(ms)", renderTime);
			ImGui::Text("editor update: %.2f(ms)", editorUpdateTime);

			ImGui::Text("draw calls: %d", gRenderStats.numDrawCalls);
			ImGui::Text("triangles drawn: %d", gRenderStats.numTris);

			ImGui::Text("primitives drawn: %d/%d", gRenderStats.drawPrimitives, gRenderStats.totalPrimitives);

			if (gWorld)
				ImGui::Text("cur time: %.2f", gWorld->CurTime());

			if (ImGui::TreeNode("Histogram"))
			{
				static bool bPause = false;
				static float values[200] = {};
				static int values_offset = 0;

				if (!bPause)
				{
					values[values_offset] = (float)(deltaTime * 1000.0);
					values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
				}

				float highestValue = 0.f;
				for (int i = 0; i < IM_ARRAYSIZE(values); i++)
					if (values[i] > highestValue)
						highestValue = values[i];

				ImGui::Checkbox("Pause", &bPause);
				ImGui::Text("Frame Time");
				ImGui::PlotHistogram("##_frameTimeHistogram", values, IM_ARRAYSIZE(values), values_offset, 0, 0.f, highestValue + 1.f, ImVec2(0, 100));

				static float values2[200] = {};
				static int values2_offset = 0;
				static bool bV2RenderTime = true;

				if (!bPause)
				{
					values2[values2_offset] = (float)(!bV2RenderTime ? updateTime : renderTime);
					values2_offset = (values2_offset + 1) % IM_ARRAYSIZE(values2);
				}

				ImGui::Text("Update Time");
				ImGui::SameLine();
				if (ImGui::SmallButton("Update"))
					bV2RenderTime = false;
				ImGui::SameLine();
				if (ImGui::SmallButton("Render"))
					bV2RenderTime = true;

				ImGui::PlotHistogram("##_frameTimeHistogram", values2, IM_ARRAYSIZE(values2), values2_offset, 0, 0.f, highestValue + 1.f, ImVec2(0, 100));

				ImGui::TreePop();
			}

			// Objects
			ImGui::Separator();

			ImGui::Text("object count: %d", CObjectManager::GetAllObjects().size());
			if (gWorld)
				ImGui::Text("entities count: %d", gWorld->GetEntities().size());

			// Resources
			ImGui::Separator();

			ImGui::Text("Asset Count: %d", CAssetManager::AssetsCount());
			ImGui::Text("Streaming Assets: %d", CAssetManager::StreamingAssetsCount());
		}
		ImGui::End();
	}

	UpdateGizmos();
}

void CEditorEngine::SetupMenu()
{
	// --- FILE ---
	CEditorMenu* menu = new CEditorMenu("New Scene", "Scene", "Ctrl+N", false);
	menu->OnClicked = []() { menuAction = MenuAction_NewScene; };
	RegisterMenu(menu, "File");

	menu = new CEditorMenu("Open Scene", "Scene", "Ctrl+O", false);
	menu->OnClicked = []() { menuAction = MenuAction_OpenScene; };
	RegisterMenu(menu, "File");

	menu = new CEditorMenu("Save", "Scene", "Ctrl+S", false);
	menu->OnClicked = []() { menuAction = MenuAction_SaveScene; };
	RegisterMenu(menu, "File");

	menu = new CEditorMenu("Save As", "Scene", FString(), false);
	menu->OnClicked = []() { menuAction = MenuAction_SaveSceneAs; };
	RegisterMenu(menu, "File");

	menu = new CEditorMenu("New Project", "Project", FString(), false);
	menu->OnClicked = []() { menuAction = MenuAction_NewProject; };
	RegisterMenu(menu, "File");

	menu = new CEditorMenu("Open Project", "Project", FString(), false);
	menu->OnClicked = []() { menuAction = MenuAction_OpenProject; };
	RegisterMenu(menu, "File");

	menu = new CEditorMenu("Close Project", "Project", FString(), false);
	menu->OnClicked = []() { gEditorEngine()->UnloadProject(); };
	RegisterMenu(menu, "File");
	menuCloseProject = menu;

	menu = new CEditorMenu("Build All", "Build", FString(), false);
	//menu->OnClicked = []() { menuAction = MenuAction_SaveSceneAs; };
	RegisterMenu(menu, "File");

	menu = new CEditorMenu("Build Lighting", "Build", FString(), false);
	//menu->OnClicked = []() { menuAction = MenuAction_SaveSceneAs; };
	RegisterMenu(menu, "File");

	menu = new CEditorMenu("Build Cubemaps", "Build", FString(), false);
	//menu->OnClicked = []() { menuAction = MenuAction_SaveSceneAs; };
	RegisterMenu(menu, "File");

	menu = new CEditorMenu("Package Engine Content", "Build", FString(), false);
	//menu->OnClicked = []() { menuAction = MenuAction_SaveSceneAs; };
	RegisterMenu(menu, "File");

	menu = new CEditorMenu("Compile Project Code", "Build", FString(), false);
	menu->OnClicked = []() {
#if _DEBUG
		gEditorEngine()->CompileProjectCode(2);
#elif _DEVELOPMENT
		gEditorEngine()->CompileProjectCode(1);
#elif _RELEASE
		gEditorEngine()->CompileProjectCode(0);
#endif
	};
	RegisterMenu(menu, "File");

	menu = new CEditorMenu("Quit", false);
	menu->OnClicked = []() { gEditorEngine()->Exit(); };
	RegisterMenu(menu, "File");

	// --- EDIT ---
	menu = new CEditorMenu("Undo", "Scene", FString(), false);
	//menu->OnClicked = []() { gEditorEngine()->Undo(); };
	RegisterMenu(menu, "Edit");

	menu = new CEditorMenu("Redo", "Scene", FString(), false);
	//menu->OnClicked = []() { gEditorEngine()->Undo(); };
	RegisterMenu(menu, "Edit");

	menu = new CEditorMenu("Copy", "Scene", FString(), false);
	//menu->OnClicked = []() { gEditorEngine()->Undo(); };
	RegisterMenu(menu, "Edit");

	menu = new CEditorMenu("Paste", "Scene", FString(), false);
	//menu->OnClicked = []() { gEditorEngine()->Undo(); };
	RegisterMenu(menu, "Edit");

	RegisterMenu(new CEditorMenu("Tools"));

	// --- VIEW ---
	menu = new CEditorMenu("Outliner", true);
	//menu->OnClicked = [=]() { gEditorEngine()->bImGuiDemo = menu->Checked(); };
	RegisterMenu(menu, "View");
	menuViewOutliner = menu;

	menu = new CEditorMenu("Asset Browser", true);
	//menu->OnClicked = [=]() { gEditorEngine()->bViewAssetBrowser = menu->Checked(); };
	RegisterMenu(menu, "View");
	menuAssetBrowser = menu;

	// --- DEBUG ---
	menu = new CEditorMenu("ImGui Demo", true);
	//menu->OnClicked = [=]() { gEditorEngine()->bViewStats = menu->Checked(); };
	RegisterMenu(menu, "Debug");
	menuImGuiDemo = menu;

	menu = new CEditorMenu("Statistics", true);
	//menu->OnClicked = [=]() { gEditorEngine()->bViewStats = menu->Checked(); };
	RegisterMenu(menu, "Debug");
	menuStatistics = menu;

	menu = new CEditorMenu("Documentation", false);
	//menu->OnClicked = []() { gEditorEngine()->Undo(); };
	RegisterMenu(menu, "Help");

	menu = new CEditorMenu("About", false);
	//menu->OnClicked = []() { gEditorEngine()->Undo(); };
	RegisterMenu(menu, "Help");

	//RegisterMenu(new CEditorMenu("Debug"));
}

void CEditorEngine::SetupEditorDocking()
{
	ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");

	if (ImGui::DockBuilderGetNode(dockspace_id) != nullptr)
		return;

	ImGui::DockBuilderAddNode(dockspace_id);

	ImGuiID dock1 = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.2f, nullptr, &dockspace_id);
	//ImGuiID dock2 = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.2f, nullptr, &dockspace_id);

	ImGuiID dock3 = ImGui::DockBuilderSplitNode(dock1, ImGuiDir_Down, 0.6f, nullptr, &dock1);
	ImGuiID dock4 = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.3f, nullptr, &dockspace_id);

	ImGui::DockBuilderDockWindow("Scene##_gameSceneViewport", dockspace_id);
	ImGui::DockBuilderDockWindow("Scene Outliner##_editorSceneOutliner", dock1);
	ImGui::DockBuilderDockWindow("Properties##_editorPropertyEditor", dock3);
	ImGui::DockBuilderDockWindow("Input/Output##_editorIOWidget", dock3);
	ImGui::DockBuilderDockWindow("Asset Browser##_editorAssetBrowser", dock4);
	ImGui::DockBuilderDockWindow("Console##_editorConsoleWidget", dock4);

	ImGui::DockBuilderFinish(dockspace_id);
}

void CEditorEngine::DrawOutliner()
{
	if (ImGui::Begin("Scene Outliner##_editorSceneOutliner", &menuViewOutliner->bChecked))
	{
		static FString searchText;
		ImGui::InputText("Search", &searchText);
		constexpr ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Hideable;

		//ImGui::TextColored(ImVec4(1, 1, 0, 1), ("Count Selected: " + FString::ToString(selectedEntities.Size())).c_str());

		ImVec2 region = ImGui::GetContentRegionAvail();
		ImVec2 cursor = ImGui::GetCursorScreenPos();

		ImGui::RenderFrame(cursor, cursor + region, ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.1f, 0.1f, 1.f)), false);

		if (ImGui::BeginTable("outliner_entities", 3, flags, region))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("Type");
			ImGui::TableSetupColumn("Visibility");
			ImGui::TableHeadersRow();

			auto ents = gWorld->GetEntities();
			for (auto& ent : ents)
			{
				if (searchText.IsEmpty() || ent.second->Name().ToLowerCase().Find(searchText.ToLowerCase()) != -1)
					OutlinerDrawEntity(ent.second);
			}

			ImGui::EndTable();
		}
	}
	ImGui::End();
}

void CEditorEngine::OutlinerDrawEntity(CEntity* ent, bool bRoot)
{
	FString type = ent->GetClass()->GetName();

	if (bRoot && ent->RootComponent()->GetParent() != nullptr)
		return;

	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	//const auto& children = ent->GetChildren();
	static TArray<CEntity*> childEnts;
	childEnts.Clear();
	for (auto& c : ent->RootComponent()->GetChildren())
		if (c->GetEntity() != ent)
			childEnts.Add(c->GetEntity());
	//for (auto& c : children)
	//	if (auto cEnt = CastChecked<CEntity>(c); cEnt)
	//		childEnts.Add(cEnt);

	int numChildren = (int)childEnts.Size();

	bool bSelected = IsEntitySelected(ent);

	//ImGui::PushStyleVar(ImGuiStyleVar_)
	if (!bSelected)
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
	else
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.21f, 0.26f, 0.38f, 1.00f));
	if (ImGui::Selectable(("##ent_select_" + ent->Name() + FString::ToString(ent->Id())).c_str(), bSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
	{
		if (ImGui::IsKeyDown(ImGuiKey_ModCtrl))
		{
			if (bSelected)
				RemoveSelectedEntity(ent);
			else
				AddSelectedEntity(ent);
		}
		else
			SetSelectedEntity(ent);
	}
	ImGui::PopStyleColor();

	if (ImGui::BeginPopupContextItem())
	{
		EntityContextMenu(ent, FVector());
		ImGui::EndPopup();
	}

	ImGui::SameLine();

	ImVec2 cursor = ImGui::GetCursorScreenPos();

	ITexture2D* entIcon = ThoriumEditor::GetThemeIcon("entities/" + ent->GetClass()->GetInternalName()); 
	if (!entIcon)
		entIcon = ThoriumEditor::GetThemeIcon("entities/CEntity");

	if (ent->type == ENTITY_DYNAMIC)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.71f, 0.81f, 1.f, 1.f));

	if (numChildren > 0)
	{
		ImGui::SetNextItemWidth(10);
		ImGui::SetCursorScreenPos(cursor - ImVec2(8, 0));
		bool bOpen = ImGui::TreeNodeEx(("##_tree_" + ent->Name()).c_str(), /*ImGuiTreeNodeFlags_SpanAllColumns |*/ ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::SameLine();

		ImGui::SetCursorScreenPos(cursor - ImVec2(-12, 3));
		ImGui::Image(TEX_VIEW(entIcon), ImVec2(20, 20));

		ImGui::SetCursorScreenPos(cursor + ImVec2(36, 0));
		ImGui::Text(ent->Name().c_str());

		if (ent->type == ENTITY_DYNAMIC)
			ImGui::PopStyleColor();

		ImGui::TableNextColumn();
		ImGui::Text(type.c_str());

		if (bOpen)
		{
			for (auto& child : childEnts)
			{
				if (child)
					OutlinerDrawEntity(child, false);
			}
			ImGui::TreePop();
		}
	}
	else
	{
		ImGui::SetCursorScreenPos(cursor - ImVec2(-12, 3));
		ImGui::Image(TEX_VIEW(entIcon), ImVec2(20, 20));

		ImGui::SetCursorScreenPos(cursor + ImVec2(36, 0));
		ImGui::Text(ent->Name().c_str());

		if (ent->type == ENTITY_DYNAMIC)
			ImGui::PopStyleColor();

		ImGui::TableNextColumn();
		ImGui::Text(type.c_str());
	}
}

void CEditorEngine::EntityContextMenu(CEntity* ent, const FVector& clickPos)
{
	if (ImGui::MenuItem("Copy", "Ctrl+C"))
		CopyEntity();
	if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, copyBuffer.dataType == CB_ENTITY))
	{
		if (clickPos.Magnitude() == 0)
			PasteEntity(ent->GetWorldPosition() + FVector((float)FMath::Random(1, 100) / 100.f));
		else
			PasteEntity(clickPos);
	}
	if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
		DupeEntity();

	if (ImGui::MenuItem("Delete", "Ctrl+X"))
	{
		ent->Delete();
		if (IsEntitySelected(ent))
			RemoveSelectedEntity(ent);
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Focus", "F"))
		FocusOnSelection();

	if (ImGui::MenuItem("Make Hidden", "H"))
		ent->bIsVisible = false;

	if (ImGui::MenuItem("Make Visible", "H"))
		ent->bIsVisible = true;

	if (ImGui::BeginMenu("Attach To"))
	{
		for (auto& _e : gWorld->GetEntities())
		{
			auto e = _e.second;
			if (e == ent)
				continue;

			if (ImGui::MenuItem((e->Name() + "##_attachToEnt_" + FString::ToString((SizeType) & *e)).c_str()))
				ent->RootComponent()->AttachTo(e->RootComponent());
		}

		ImGui::EndMenu();
	}
}

void CEditorEngine::DoEntityShortcuts()
{
	if (selectedEntities.Size() == 0)
		return;

	if (ImGui::IsKeyReleased(ImGuiKey_F))
		FocusOnSelection();

	if (ImGui::IsKeyReleased(ImGuiKey_H))
	{
		bool bVis = selectedEntities[0]->bIsVisible;
		for (auto& obj : selectedEntities)
		{
			obj->bIsVisible = !bVis;
		}
	}

	if (ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyReleased(ImGuiKey_X))
	{
		for (auto& ent : selectedEntities)
		{
			ent->Delete();
			if (IsEntitySelected(ent))
				RemoveSelectedEntity(ent);
		}
	}
}

// Includes for all entity types
#include "Game/Entities/SunLightEntity.h"
#include "Game/Entities/PointLightEntity.h"
#include "Game/Entities/PlayerStart.h"
#include "Game/Entities/ModelEntity.h"

void CEditorEngine::DrawObjectCreateMenu()
{
	auto* scene = gWorld->GetRenderScene();
	FVector entPos = scene->GetPrimaryCamera() ? scene->GetPrimaryCamera()->position + scene->GetPrimaryCamera()->rotation.Rotate(FVector(0, 0, 1)) : FVector();

	if (ImGui::MenuItem("Entity"))
		gWorld->CreateEntity<CEntity>("Entity", entPos);

	if (ImGui::MenuItem("Player Start"))
		gWorld->CreateEntity<CPlayerStart>("Player Start", entPos);

	if (ImGui::BeginMenu("Shapes"))
	{
		if (ImGui::MenuItem("Cube"))
		{
			auto ent = gWorld->CreateEntity<CModelEntity>("Cube", entPos);
			ent->SetModel("models/Cube.thmdl");
		}

		if (ImGui::MenuItem("Sphere"))
		{
			auto ent = gWorld->CreateEntity<CModelEntity>("Sphere", entPos);
			ent->SetModel("models/Sphere.thmdl");
		}

		if (ImGui::MenuItem("Cylinder"))
		{
			auto ent = gWorld->CreateEntity<CModelEntity>("Cube", entPos);
			ent->SetModel("models/Cube.thmdl");
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Lights"))
	{
		if (ImGui::MenuItem("Sun Light"))
			gWorld->CreateEntity<CSunLightEntity>("Sun Light", entPos);

		if (ImGui::MenuItem("Point Light"))
			gWorld->CreateEntity<CPointLightEntity>("Point Light", entPos);

		if (ImGui::MenuItem("Spot Light"))
			gWorld->CreateEntity<CPointLightEntity>("Point Light", entPos);

		ImGui::EndMenu();
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Select Class..."))
		ThoriumEditor::SelectClass("SceneCreateEntity", CEntity::StaticClass());
}

void CEditorEngine::DoEntRightClick()
{
	static FVector clickPos;
	if (ImGui::BeginPopup("popupEntViewportContext"))
	{
		if (selectedEntities.Size() > 0)
			EntityContextMenu(selectedEntities[0], clickPos);
		ImGui::EndPopup();
	}

	static ImVec2 mousePos;
	if (ImGui::IsItemHovered())
	{
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			mousePos = ImGui::GetIO().MousePos;

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && mousePos.x == ImGui::GetIO().MousePos.x && mousePos.y == ImGui::GetIO().MousePos.y)
		{
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
					SetSelectedEntity(ent);
					ImGui::OpenPopup("popupEntViewportContext");

					clickPos = hit.position;
				}
			}
		}
	}
}
