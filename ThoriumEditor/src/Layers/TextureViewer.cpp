
#include "TextureViewer.h"
#include "EditorEngine.h"
#include "Resources/ResourceManager.h"
#include "Resources/ModelAsset.h"
#include "Resources/Material.h"
#include "Game/World.h"
#include "Game/Entities/ModelEntity.h"
#include "Game/Components/PointLightComponent.h"
#include "Game/Components/SkyboxComponent.h"
#include "Rendering/RenderScene.h"
#include "Rendering/RenderProxies.h"

#include "AssetBrowserWidget.h"
#include "FileDialogs.h"

#include "Platform/Windows/DirectX/DirectXFrameBuffer.h"
#include "Platform/Windows/DirectX/DirectXTexture.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/ImGui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "EditorWidgets.h"

class FTextureOpenAction : public FAssetBrowserAction
{
public:
	FTextureOpenAction()
	{
		type = BA_OPENFILE;
		targetClass = (FAssetClass*)CTexture::StaticClass();
	}

	void Invoke(FBrowserActionData* data) override
	{
		auto* editor = gEditorEngine()->AddLayer<CTextureViewer>();
		editor->SetTexture(CResourceManager::GetResource<CTexture>(data->file->Path()));
	}
} static FTextureOpenAction_instance;

CTextureViewer::CTextureViewer()
{
	//scene = CreateObject<CWorld>();
	//scene->InitWorld(CWorld::InitializeInfo().CreateRenderScene(true).RegisterForRendering(true));

	//modelEnt = scene->CreateEntity<CModelEntity>();
	//modelEnt->SetModel(L"models\\Cube.thmdl");

	//framebuffer = gRenderer->CreateFrameBuffer(1280, 720, THTX_FORMAT_RGBA8_UINT);
	//depthbuffer = gRenderer->CreateDepthBuffer({ 1280, 720, TH_DBF_D24_S8, 1, false });

	//camera = new CCameraProxy();
	//camera->position = { 0, 0, -2 };
	//camera->fov = 36.9f;
	//camera->rotation = FQuaternion::EulerAngles(FVector(0, 180, 0).Radians());

	//matUnlit = CreateObject<CMaterial>();
	//matUnlit->SetShader("Unlit");
}

void CTextureViewer::SetTexture(CTexture* t)
{
	tex = t;

	reimportSettings.format = tex->Format();
	reimportSettings.filter = tex->FilterType();
	reimportSettings.numMipMaps = tex->MipMapCount();

	CFStream sdkStream = t->File()->GetSdkStream("rb");
	if (sdkStream.IsOpen())
	{
		FString file;
		sdkStream >> file;
		texSourceFile = file;
	}
}

void CTextureViewer::OnUpdate(double dt)
{
	//int w, h;
	//framebuffer->GetSize(w, h);
	//if (w != viewportWidth || h != viewportHeight)
	//{
	//	framebuffer->Resize(FMath::Max(viewportWidth, 16), FMath::Max(viewportHeight, 16));
	//	depthbuffer->Resize(FMath::Max(viewportWidth, 16), FMath::Max(viewportHeight, 16));
	//}

	//scene->Update(dt);
	//scene->GetRenderScene()->SetFrameBuffer(framebuffer);
	//scene->GetRenderScene()->SetDepthBuffer(depthbuffer);

	//scene->SetPrimaryCamera(camera);

	//matUnlit->SetTexture("vBaseColor", tex);
	//modelEnt->modelComp->SetMaterial(matUnlit, 0);
}

void CTextureViewer::OnUIRender()
{
	FString title = "Texture Viewer";
	if (tex && tex->File())
		title += " - " + tex->File()->Name();
	else if (tex)
		title += " - ERROR: Texture has no file!";

	if (tex && !bSaved)
		title += '*';

	title += "###textureViewer_" + FString::ToString((SizeType)this);

	bool bOpen = true;
	ImGui::SetNextWindowSize(ImVec2(900, 620), ImGuiCond_FirstUseEver);

	if (ImGui::Begin(title.c_str(), &bOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImVec2 size = ImGui::GetContentRegionAvail();
		sizeR = size.x - sizeL;

		ImGui::Splitter("##textureViewerSplitter", false, 4.f, &sizeL, &sizeR);

		if (ImGui::BeginChild("texproperties", ImVec2(sizeL, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding))
		{
			ImVec2 wndSize = ImGui::GetContentRegionAvail();
			ImGui::Text("Size: " + FString::ToString(tex->GetWidth()) + "x" + FString::ToString(tex->GetHeight()));

			ImGui::SameLine();
			if (ImGui::GetCursorPosX() < wndSize.x / 2)
				ImGui::SetCursorPosX(wndSize.x / 2);

			const char* formatNames[] = {
				"R8",
				"RG8",
				"RGB8",
				"RGBA8",
				"RGBA16 float",
				"RGBA32 float",
				"DXT1",
				"DXT5"
			};

			FString formatText = "Format: ";
			formatText += formatNames[(int)tex->Format()];
			ImGui::Text(formatText.c_str());

			ImGui::Text("Mip Maps: " + FString::ToString(tex->MipMapCount()));

			ImGui::SameLine();
			if (ImGui::GetCursorPosX() < wndSize.x / 2)
				ImGui::SetCursorPosX(wndSize.x / 2);

			ImGui::Text("Size: " + FString::ToString(tex->DataSize() / 1024) + "Kb");

			const char* filterNames[] = {
				"Linear",
				"Point",
				"Anisotropic"
			};

			ImGui::Text(FString("Filtering mode: ") + filterNames[tex->FilterType()]);

			ImGui::SameLine();
			if (ImGui::GetCursorPosX() < wndSize.x / 2)
				ImGui::SetCursorPosX(wndSize.x / 2);

			ImGui::Text("Display Size: " + FString::ToString(int(tex->GetWidth() * scale)) + "x" + FString::ToString(int(tex->GetHeight() * scale)));

			bool bEditable = !texSourceFile.IsEmpty();

			if (!bEditable)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.4f));
				ImGui::Text("No source file");
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::TextWrapped(texSourceFile.c_str());

				if (ImGui::Button("Reimport"))
					UpdateTex();
			}

			if (ImGui::BeginCombo("Format", formatNames[reimportSettings.format]))
			{
				if (bEditable)
				{
					for (int i = 0; i < 8; i++)
						if (ImGui::Selectable(formatNames[i], reimportSettings.format == i))
							reimportSettings.format = (ETextureAssetFormat)i;
				}

				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("Filter Mode", filterNames[reimportSettings.filter]))
			{
				if (bEditable)
				{
					for (int i = 0; i < 3; i++)
						if (ImGui::Selectable(filterNames[i], reimportSettings.filter == i))
							reimportSettings.filter = (ETextureFilter)i;
				}

				ImGui::EndCombo();
			}

			ImGui::DragInt("MipMaps", (int*)&reimportSettings.numMipMaps, 0.25f, 1, 12, "%d", bEditable ? 0 : ImGuiSliderFlags_ReadOnly);
		}
		ImGui::EndChild();

		ImGui::SameLine();
		
		if (ImGui::BeginChild("texView", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_HorizontalScrollbar))
		{
			auto wndSize = ImGui::GetContentRegionAvail();
			auto cursorPos = ImGui::GetCursorScreenPos();
			viewportX = cursorPos.x;
			viewportY = cursorPos.y;

			float aspectRatio = 1.f;
			if (tex)
				tex->Load(0);

			if (tex && tex->GetTextureObject())
			{
				aspectRatio = (float)tex->GetWidth() / (float)tex->GetHeight();

				//float size = wndSize.x > wndSize.y ? wndSize.y : wndSize.x;

				DirectXTexture2D* fb = (DirectXTexture2D*)tex->GetTextureObject();
				ImGui::Image(fb->view, { tex->GetWidth() * scale, tex->GetHeight() * scale });

				ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);

				if (ImGui::GetIO().MouseWheel > 0)
					scale = FMath::Min(scale + 0.125f, 2.f);
				if (ImGui::GetIO().MouseWheel < 0)
					scale = FMath::Max(scale - 0.125f, 0.125f);
			}

			viewportWidth = FMath::Max((int)wndSize.x, 32);
			viewportHeight = FMath::Max((int)wndSize.y, 32);
		}
		ImGui::EndChild();
	}
	ImGui::End();

	if (!bOpen)
	{
		gEditorEngine()->PollRemoveLayer(this);
	}
}

void CTextureViewer::OnDetach()
{
	//scene->Delete();
	//delete framebuffer;
	//delete depthbuffer;
	//delete camera;
}

void CTextureViewer::UpdateTex()
{
	tex->Import(texSourceFile, reimportSettings);
}
