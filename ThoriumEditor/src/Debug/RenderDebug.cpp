
#include "RenderDebug.h"
#include "EditorEngine.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderScene.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Imgui/imgui_thorium.h"

#include "Platform/Windows/DirectX/DirectXFrameBuffer.h"
#include "Platform/Windows/DirectX/DirectXTexture.h"

REGISTER_EDITOR_LAYER(CRenderDebugger, "Debug/Render Debug", nullptr, false, false)

#define TEX_VIEW(tex) ((DirectXTexture2D*)tex)->view
#define FB_VIEW(tex) ((DirectXFrameBuffer*)tex)->view
#define DEPTH_VIEW(tex) ((DirectXDepthBuffer*)tex)->view

void CRenderDebugger::OnUIRender()
{
	if (ImGui::Begin("Render Debug", &bEnabled))
	{
		if (ImGui::BeginTabBar("renderDebugTabs"))
		{
			if (ImGui::BeginTabItem("Scene"))
			{
				TabScene();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Lights"))
			{
				TabLights();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Primitives"))
			{
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Volumes"))
			{
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

static const char* framebufferLabels[] = {
	"Color",
	"Normal",
	"Material",
	"Albedo",
	"GBufferD",
	"Ambient Occlusion",
	"Bloom1",
	"Bloom2",
	"Bloom3",
	"Bloom4",
	"Depth",
};

static SizeType framebufferOffsets[] = {
	offsetof(CRenderScene, colorBuffer),
	offsetof(CRenderScene, GBufferA),
	offsetof(CRenderScene, GBufferB),
	offsetof(CRenderScene, GBufferC),
	offsetof(CRenderScene, GBufferD),
	offsetof(CRenderScene, aoBuffer),
	offsetof(CRenderScene, bloomBuffersY[0]),
	offsetof(CRenderScene, bloomBuffersY[1]),
	offsetof(CRenderScene, bloomBuffersY[2]),
	offsetof(CRenderScene, bloomBuffersY[3]),
	offsetof(CRenderScene, depthTex),
};

void CRenderDebugger::TabScene()
{
	CRenderScene* scene = gWorld->GetRenderScene();
	if (!scene)
	{
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "the active world doesn't contain a render scene!");
		return;
	}

	if (ImGui::CollapsingHeader("Info"))
	{
		ImGui::Text("time: %.1f", scene->GetTime());
		ImGui::Text("screen percentage: %d%%", (int)scene->ScreenPercentage());
		ImGui::Text("framebuffer size: %dx - %dy", scene->GetFrameBufferWidth(), scene->GetFrameBufferHeight());
	}

	if (ImGui::CollapsingHeader("Framebuffers"))
	{
		static int curFB = 0;

		if (ImGui::BeginCombo("Framebuffer", framebufferLabels[curFB]))
		{
			for (int i = 0; i < 11; i++)
			{
				if (ImGui::Selectable(framebufferLabels[i], i == curFB))
					curFB = i;
			}

			ImGui::EndCombo();
		}

		IFrameBuffer* fb = *(IFrameBuffer**)(((SizeType)scene) + framebufferOffsets[curFB]);
		//IFrameBuffer* fb = scene->colorBuffer;
		ITexture2D* depth = (ITexture2D*)fb;

		int w, h;

		if (curFB < 10)
		{
			fb->GetSize(w, h);
			ImGui::Image(FB_VIEW(fb), ImVec2(w, h));
		}
		else
		{
			depth->GetSize(w, h);
			ImGui::Image(TEX_VIEW(depth), ImVec2(w, h));
		}
	}
}

void CRenderDebugger::TabLights()
{
	CRenderScene* scene = gWorld->GetRenderScene();
	if (!scene)
	{
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "the active world doesn't contain a render scene!");
		return;
	}

	const TArray<CLightProxy*>& lights = scene->GetLights();


}

