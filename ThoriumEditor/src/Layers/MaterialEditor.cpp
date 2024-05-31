
#include <string>
#include "EditorEngine.h"
#include "MaterialEditor.h"
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

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/ImGui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "EditorWidgets.h"

REGISTER_EDITOR_LAYER(CMaterialEditor, "Tools/Material Editor", nullptr, true, false)

class FMaterialOpenAction : public FAssetBrowserAction
{
public:
	FMaterialOpenAction()
	{
		type = BA_OPENFILE;
		targetClass = (FAssetClass*)CMaterial::StaticClass();
	}

	void Invoke(FBrowserActionData* data) override
	{
		auto* editor = gEditorEngine()->AddLayer<CMaterialEditor>();
		editor->SetMaterial(CResourceManager::GetResource<CMaterial>(data->file->Path()));
	}
} static FMaterialOpenAction_instance;

CMaterialEditor::CMaterialEditor()
{
	previewModel = CResourceManager::GetResource<CModelAsset>("models/Sphere.thmdl");

	scene = CreateObject<CWorld>();
	scene->InitWorld(CWorld::InitializeInfo().CreateRenderScene(true).RegisterForRendering(true));

	auto skyEnt = scene->CreateEntity<CEntity>();
	skyEnt->AddComponent<CSkyboxComponent>("skybox");

	modelEnt = scene->CreateEntity<CModelEntity>();
	modelEnt->SetModel(previewModel);

	fbView = gRenderer->CreateFrameBuffer(1280, 720, TEXTURE_FORMAT_RGBA8_UNORM);
	scene->GetRenderScene()->SetFrameBuffer(fbView);
	//dbView = gRenderer->CreateDepthBuffer({ 1280, 720, TH_DBF_D24_S8, 1, false });

	camera = new CCameraProxy();
	camera->position = { 0, 0, -2 };
	scene->SetPrimaryCamera(camera);

	light1 = modelEnt->AddComponent<CPointLightComponent>("Light1");
	light1->AttachTo(modelEnt->RootComponent());
	light1->SetPosition({ -1, 1, -1 });

	light2 = modelEnt->AddComponent<CPointLightComponent>("Light2");
	light2->AttachTo(modelEnt->RootComponent());
	light2->SetPosition({ 1, 0, -1 });

	openMatId = "MatEditorOpenMat" + FString::ToString((SizeType)this);
	saveMatId = "MatEditorSaveMat" + FString::ToString((SizeType)this);
}

void CMaterialEditor::SetMaterial(CMaterial* m, bool bNew)
{
	mat = m;
	bSaved = !bNew;

	UpdateCache();
}

void CMaterialEditor::OnUpdate(double dt)
{
	int w, h;
	fbView->GetSize(w, h);
	if (w != viewportWidth || h != viewportHeight)
	{
		fbView->Resize(FMath::Max(viewportWidth, 16), FMath::Max(viewportHeight, 16));
	}

	scene->Update(dt);
	scene->GetRenderScene()->SetFrameBuffer(fbView);
	//scene->GetRenderScene()->SetDepthBuffer(dbView);

	scene->SetPrimaryCamera(camera);

	modelEnt->modelComp->SetMaterial(mat, 0);
}

void CMaterialEditor::OnUIRender()
{
	FString title = "Material Editor";
	if (mat && mat->File())
		title += " - " + mat->File()->Name();
	else if (mat)
		title += " - New Material";

	if (mat && !bSaved)
		title += '*';

	title += "###materialEditor_" + FString::ToString((SizeType)this);

	bool bOpen = true;
	ImGui::SetNextWindowSize(ImVec2(1200, 720), ImGuiCond_FirstUseEver);

	FString f;
	FString m;
	if (ThoriumEditor::AcceptFile(openMatId, &f) && !f.IsEmpty())
	{
		CMaterial* m = CResourceManager::GetResource<CMaterial>(f);
		if (m)
			SetMaterial(m);
	}
	if (ThoriumEditor::AcceptFile(saveMatId, &f, &m) && !f.IsEmpty())
	{
		CResourceManager::RegisterNewResource(mat, f, m);
		mat->Save();
		bSaved = true;
	}

	int openPopup = 0;
	if (ImGui::Begin(title.c_str(), &bOpen, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl+N"))
					openPopup = 1;
				if (ImGui::MenuItem("Open", "Ctrl+O"))
					openPopup = 2;
				if (ImGui::MenuItem("Save", "Ctrl+S"))
					openPopup = 3;
				ImGui::MenuItem("Save As");

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				ImGui::MenuItem("Undo");
				ImGui::MenuItem("Redo");
				if (ImGui::MenuItem("Revert"))
					Revert();

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::IsKeyDown(ImGuiKey_ModCtrl))
			openPopup = 3;

		if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_N) && ImGui::IsKeyDown(ImGuiKey_ModCtrl))
			OnSaveNewMaterial(1);

		ImVec2 size = ImGui::GetContentRegionAvail();
		sizeR = size.x - sizeL;

		ImGui::Splitter("##assetBorwserSplitter", false, 4.f, &sizeL, &sizeR);

		if (ImGui::BeginChild("mateditorProperties", ImVec2(sizeL, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding))
		{
			if (!mat)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.4f));
				ImGui::Text("No material loaded");
				ImGui::PopStyleColor();
			}
			else
			{
				const auto& shaders = CShaderSource::GetAllShaders();

				if (ImGui::BeginCombo("Shader", mat->GetShaderSource()->shaderName.c_str()))
				{
					for (auto& s : shaders)
					{
						if (s->type == CShaderSource::ST_INTERNAL)
							continue;

						bool bSelected = s == mat->GetShaderSource();
						if (ImGui::Selectable(s->shaderName.c_str(), bSelected))
						{
							mat->SetShader(s->shaderName);
							UpdateCache();
							bSaved = false;
						}
						if (bSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				ImGui::BeginTabBar("materialTabBar");

				if (ImGui::BeginTabItem("Properties"))
				{
					DrawProperties();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Settings"))
				{
					if (ImGui::BeginTable("matPropertiesTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
					{
						CShaderSource* shader = mat->GetShaderSource();

						ImGui::BeginDisabled(shader->type != CShaderSource::ST_FORWARD_DEFERRED);
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Preferred Render Pass");
						ImGui::TableNextColumn();
						const char* passNames[] = {
							"Deferred",
							"Opaque Deferred - Transparent Forward",
							"Forward"
						};

						if (ImGui::BeginCombo("##comboPreferredPass", passNames[(int)mat->preferredRenderPass]))
						{
							for (int i = 0; i < 3; i++)
							{
								if (ImGui::Selectable(passNames[i], i == mat->preferredRenderPass))
								{
									mat->preferredRenderPass = i;
									bSaved = false;
								}
							}

							ImGui::EndCombo();
						}

						ImGui::EndDisabled();
						
						ImGui::BeginDisabled(shader->type != CShaderSource::ST_FORWARD && shader->type != CShaderSource::ST_FORWARD_DEFERRED);
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Force Transparent");
						ImGui::TableNextColumn();
						if (ImGui::Checkbox("##forceTransparent", &mat->bForceTransparentPass))
							bSaved = false;
						ImGui::EndDisabled();

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Receive Shadows");
						ImGui::TableNextColumn();
						if (ImGui::Checkbox("##recieveShadows", &mat->bReceiveShadows))
							bSaved = false;

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Cast Shadows");
						ImGui::TableNextColumn();
						if (ImGui::Checkbox("##castShadows", &mat->bCastShadows))
							bSaved = false;

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Depth test");
						ImGui::TableNextColumn();
						if (ImGui::Checkbox("##depthTest", &mat->bDepthTest))
							bSaved = false;

						ImGui::EndTable();
					}
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		auto wndSize = ImGui::GetContentRegionAvail();
		auto cursorPos = ImGui::GetCursorScreenPos();
		viewportX = cursorPos.x;
		viewportY = cursorPos.y;

		DirectXFrameBuffer* fb = (DirectXFrameBuffer*)fbView;
		ImGui::Image(fb->view, { wndSize.x, wndSize.y });

		viewportWidth = FMath::Max((int)wndSize.x, 32);
		viewportHeight = FMath::Max((int)wndSize.y, 32);
	}
	ImGui::End();

	if (openPopup == 1)
	{
		if (!bSaved && mat)
		{
			saveCallback = &CMaterialEditor::OnSaveNewMaterial;
			ImGui::OpenPopup("Continue without saving?##_MATEDITCLOSE");
		}
		else
			OnSaveNewMaterial(1);
	}
	else if (openPopup == 2)
	{
		if (!bSaved && mat)
		{
			saveCallback = &CMaterialEditor::OnSaveOpenMaterial;
			ImGui::OpenPopup("Continue without saving?##_MATEDITCLOSE");
		}
		else
			ThoriumEditor::OpenFile(openMatId, (FAssetClass*)CMaterial::StaticClass());
	}
	else if (openPopup == 3)
	{
		SaveMat();
	}

	if (!bOpen)
	{
		bWantsToClose = true;
		if (!bSaved && mat)
		{
			saveCallback = &CMaterialEditor::OnSaveExit;
			ImGui::OpenPopup("Continue without saving?##_MATEDITCLOSE");
		}
		else
			gEditorEngine()->PollRemoveLayer(this);
	}

	int savePopupResult = -1;

	if (ImGui::BeginPopupModal("Continue without saving?##_MATEDITCLOSE"))
	{
		ImGui::Text("are you sure you want to continue without saving?");

		if (ImGui::Button("Save"))
		{
			ImGui::CloseCurrentPopup();
			SaveMat();
			savePopupResult = 1;
		}

		ImGui::SameLine();

		if (ImGui::Button("Don't Save"))
		{
			//gEditorEngine()->PollRemoveLayer(this);
			Revert();
			ImGui::CloseCurrentPopup();
			savePopupResult = 2;
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel"))
		{
			//bWantsToClose = false;
			ImGui::CloseCurrentPopup();
			savePopupResult = 0;
		}

		ImGui::EndPopup();
	}

	if (savePopupResult != -1 && saveCallback)
	{
		(this->*saveCallback)(savePopupResult);
		saveCallback = nullptr;
	}
}

void CMaterialEditor::OnDetach()
{
	scene->Delete();
	delete fbView;
	delete camera;
}

void CMaterialEditor::SaveMat()
{
	if (!mat)
		return;

	if (!mat->File())
	{
		ThoriumEditor::SaveFile(saveMatId, (FAssetClass*)CMaterial::StaticClass());
		return;
	}

	mat->Save();
	bSaved = true;
}

void CMaterialEditor::DrawProperties()
{
	//for (auto& t : mat->textures)
	//{
	//	TObjectPtr<CTexture>* tex = &t.tex;
	//	if (!t.bIsCustom)
	//		if (ImGui::AssetPtrWidget(t.name.c_str(), &tex, 1))
	//			bSaved = false;
	//}

	if (ImGui::BeginTable("matPropertiesTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
	{
		FString curCat = cache.size() > 0 ? cache.begin()->first : "";
		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FrameDontExpand;
		bool bOpen = ImGui::TableTreeHeader(curCat.c_str(), treeFlags);

		for (auto& p : cache)
		{
			if (curCat != p.first)
			{
				curCat = p.first;
				if (bOpen)
					ImGui::TreePop();
				bOpen = ImGui::TableTreeHeader(curCat.c_str(), treeFlags);
			}

			if (!bOpen)
				continue;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 4));
			ImGui::Text(p.second.type == 0 ? p.second.prop->name.c_str() : p.second.tex->name.c_str());
			ImGui::TableNextColumn();

			if (p.second.type == 1)
			{
				auto* t = p.second.tex;
				TObjectPtr<CTexture>* tex = &p.second.tex->tex;
				bool bSwitch = false;
				if (!p.second.tex->bIsCustom)
				{
					if (ImGui::AssetPtrWidget(("##_" + p.second.tex->name).c_str(), &tex, 1))
						bSaved = false;
					ImGui::SameLine();
					bSwitch = ImGui::Button(("Color##" + t->name).c_str());
				}
				else
				{
					float col[4];
					col[0] = (float)t->color[0] / 255.f;
					col[1] = (float)t->color[1] / 255.f;
					col[2] = (float)t->color[2] / 255.f;
					col[3] = (float)t->color[3] / 255.f;
					if (ImGui::ColorEdit4(("##_" + t->name).c_str(), col))
					{
						t->color[0] = int(col[0] * 255.f);
						t->color[1] = int(col[1] * 255.f);
						t->color[3] = int(col[3] * 255.f);
						t->color[2] = int(col[2] * 255.f);
						
						uint8 data[] = {
							t->color[0], t->color[1], t->color[2], t->color[3],
							t->color[0], t->color[1], t->color[2], t->color[3],
							t->color[0], t->color[1], t->color[2], t->color[3],
							t->color[0], t->color[1], t->color[2], t->color[3]
						};

						if (!t->tex)
							t->tex = CreateObject<CTexture>();
						t->tex->Init(data, 2, 2);
						bSaved = false;
					}
					bSwitch = ImGui::Button(("Texture##" + t->name).c_str());
				}
				if (bSwitch)
				{
					t->bIsCustom = !t->bIsCustom;
					t->tex = nullptr;
					bSaved = false;
				}
			}
			if (p.second.type == 0)
			{
				CMaterial::MatProperty* prop = p.second.prop;
				FShaderProperty* sp = mat->GetShaderProperty(*prop);

				switch (prop->type)
				{
				case FShaderProperty::BOOL:
					if (ImGui::Checkbox(("##_matPBool" + prop->name).c_str(), (bool*)&prop->pBool))
					{
						prop->bRequiresUpdate = true;
						bSaved = false;
					}
					break;
				case FShaderProperty::INT:
					if (ImGui::DragInt(("##_matPInt" + prop->name).c_str(), &prop->pInt))
					{
						prop->bRequiresUpdate = true;
						bSaved = false;
					}
					break;
				case FShaderProperty::FLOAT:
					if (ImGui::DragFloat(("##_matPFloat" + prop->name).c_str(), &prop->pFloat, 0.1f))
					{
						prop->bRequiresUpdate = true;
						bSaved = false;
					}
					break;
				case FShaderProperty::VEC3:
					if (sp->uiType != FShaderProperty::COLOR)
					{
						float size = ImGui::GetContentRegionAvail().x / 3 - 15.f;
						ImGui::SetNextItemWidth(size);
						if (ImGui::DragFloat(("X##_matPVec3X" + prop->name).c_str(), &prop->pVec3[0], 0.1f))
						{
							bSaved = false;
							prop->bRequiresUpdate = true;
						}
						ImGui::SameLine();
						ImGui::SetNextItemWidth(size);
						if (ImGui::DragFloat(("Y##_matPVec3Y" + prop->name).c_str(), &prop->pVec3[1], 0.1f))
						{
							bSaved = false;
							prop->bRequiresUpdate = true;
						}
						ImGui::SameLine();
						ImGui::SetNextItemWidth(size);
						if (ImGui::DragFloat(("Z##_matPVec3Z" + prop->name).c_str(), &prop->pVec3[2], 0.1f))
						{
							bSaved = false;
							prop->bRequiresUpdate = true;
						}
					}
					else
					{
						if (ImGui::ColorEdit3(("##_matPColor3" + prop->name).c_str(), prop->pVec3))
						{
							prop->bRequiresUpdate = true;
							bSaved = false;
						}
					}
					break;
				case FShaderProperty::VEC4:
					if (sp->uiType != FShaderProperty::COLOR)
					{
						bool bUpdate = false;
						float size = ImGui::GetContentRegionAvail().x / 3 - 20.f;
						ImGui::SetNextItemWidth(size);
						bUpdate = ImGui::DragFloat(("X##_matPVec4X" + prop->name).c_str(), &prop->pVec4[0], 0.1f) || bUpdate;
						ImGui::SameLine();
						ImGui::SetNextItemWidth(size);
						bUpdate = ImGui::DragFloat(("Y##_matPVec4Y" + prop->name).c_str(), &prop->pVec4[1], 0.1f) || bUpdate;
						ImGui::SameLine();
						ImGui::SetNextItemWidth(size);
						bUpdate = ImGui::DragFloat(("Z##_matPVec4Z" + prop->name).c_str(), &prop->pVec4[2], 0.1f) || bUpdate;
						ImGui::SameLine();
						ImGui::SetNextItemWidth(size);
						bUpdate = ImGui::DragFloat(("W##_matPVec4W" + prop->name).c_str(), &prop->pVec4[3], 0.1f) || bUpdate;

						if (bUpdate)
							prop->bRequiresUpdate = true;
					}
					else
					{
						if (ImGui::ColorEdit4(("##_matPColor4" + prop->name).c_str(), prop->pVec4))
						{
							prop->bRequiresUpdate = true;
							bSaved = false;
						}
					}
					break;
				}
			}
		}

		if (bOpen)
			ImGui::TreePop();

		ImGui::EndTable();
	}

}

void CMaterialEditor::UpdateCache()
{
	cache.clear();

	if (!mat)
		return;

	for (auto& t : mat->textures)
	{
		auto* st = mat->GetShaderTexture(t);
		PropertyCache tc;
		tc.type = 1;
		tc.tex = &t;
		cache.insert(std::make_pair(st->UiGroup.c_str(), tc));
	}

	for (auto& p : mat->properties)
	{
		auto* sp = mat->GetShaderProperty(p);
		PropertyCache pc;
		pc.type = 0;
		pc.prop = &p;
		cache.insert(std::make_pair(sp->UiGroup.c_str(), pc));
	}
}

void CMaterialEditor::Revert()
{
	if (mat->File())
		mat->Init();
	bSaved = true;
}

void CMaterialEditor::OnSaveOpenMaterial(int result)
{
	if (result != 0)
		ThoriumEditor::OpenFile(openMatId, (FAssetClass*)CMaterial::StaticClass());
}

void CMaterialEditor::OnSaveExit(int result)
{
	// Cancel
	if (result == 0)
		bWantsToClose = false;
	else
		gEditorEngine()->PollRemoveLayer(this);
}

void CMaterialEditor::OnSaveNewMaterial(int result)
{
	if (result != 0)
	{
		CMaterial* mat = CreateObject<CMaterial>();
		mat->SetShader("Simple");
		SetMaterial(mat, true);
	}
}
