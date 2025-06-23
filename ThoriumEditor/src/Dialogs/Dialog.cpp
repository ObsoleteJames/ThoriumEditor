
#include "Dialog.h"
#include "Engine.h"
#include "Window.h"
#include "Rendering/GraphicsInterface.h"
#include "ThemeManager.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

#include "GLFW/glfw3.h"

CDialogWnd::CDialogWnd(const FString& t, EDialogFlags f /*= 0*/) : title(t), flags(f)
{
}

int CDialogWnd::Exec()
{
	int w, h;
	CEngine::GetMonitorSize(0, &w, &h);

	int x = (w - windowSize.x) / 2;
	int y = (h - windowSize.y) / 2;

	window = new CWindow(windowSize.x, windowSize.y, x, y, title, CWindow::WM_WINDOWED);
	
	swapChain = gGHI->CreateSwapChain(window);
	window->SetSwapChain(swapChain);

	auto* mainContext = ImGui::GetCurrentContext();
	auto* mainWindow = mainContext->CurrentWindow;
	context = ImGui::CreateContext(mainContext->IO.Fonts);
	ImGui::SetCurrentContext(context);

	gGHI->InitImGui(window);

	ThoriumEditor::SetTheme(ThoriumEditor::Theme().name);

	while (!bFinished)
	{
		CWindow::PollEvents();

		if (window->WantsToClose() && (flags & EDialogFlags_IgnoreClose) == 0)
			Finish(0);

		int w, h;
		window->GetSize(w, h);
		windowSize = { (float)w, (float)h };

		gGHI->ImGuiBeginFrame();

		ImGui::SetNextWindowSize({ windowSize.x, windowSize.y }, ImGuiCond_Always);
		ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Always);
		//ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

		Render();

		ImGui::PopStyleVar(1);
		
		swapChain->GetDepthBuffer()->Clear();
		gGHI->SetFrameBuffer(swapChain->GetFrameBuffer(), swapChain->GetDepthBuffer());
		gGHI->ImGuiRender();

		window->Present(1, 0);
	}

	delete swapChain;
	delete window;
	ImGui::DestroyContext(context);

	ImGui::SetCurrentContext(mainContext);

	return returnCode;
}

void CDialogWnd::Finish(int r)
{
	bFinished = true;
	returnCode = r;
}

void CDialogWnd::Render()
{
}
