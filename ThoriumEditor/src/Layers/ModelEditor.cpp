
#include "ModelEditor.h"
#include "EditorEngine.h"
#include "Game/World.h"
#include "Game/Entities/ModelEntity.h"
#include "Game/Components/SkyboxComponent.h"
#include "Game/Components/PointLightComponent.h"
#include "Game/Input/InputManager.h"
#include "Rendering/RenderScene.h"
#include "Rendering/DebugRenderer.h"
#include "AssetBrowserWidget.h"
#include <Util/KeyValue.h>

#include "FileDialogs.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "EditorEngine.h"
#include "CameraController.h"

#include "Platform/Windows/DirectX/DirectXFrameBuffer.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/ImGui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "ImGui/imgui_thorium_math.h"
#include "EditorWidgets.h"

REGISTER_EDITOR_LAYER(CModelEditor, "Tools/Model Editor", nullptr, true, false)

class FModelOpenAction : public FAssetBrowserAction
{
public:
	FModelOpenAction()
	{
		type = BA_OPENFILE;
		targetClass = (FAssetClass*)CModelAsset::StaticClass();
	}

	void Invoke(FBrowserActionData* data) override
	{
		auto* editor = gEditorEngine()->AddLayer<CModelEditor>();
		editor->SetModel(CResourceManager::GetResource<CModelAsset>(data->file->Path()));
	}
} static FModelOpenAction_instance;

CModelEditor::CModelEditor()
{
	scene = CreateObject<CWorld>();
	scene->InitWorld(CWorld::InitializeInfo().CreateRenderScene(true).RegisterForRendering(true));

	auto skyEnt = scene->CreateEntity<CEntity>();
	skyEnt->AddComponent<CSkyboxComponent>("skybox");

	modelEnt = scene->CreateEntity<CModelEntity>();

	framebuffer = gRenderer->CreateFrameBuffer(1280, 720, TEXTURE_FORMAT_RGBA8_UNORM);
	scene->GetRenderScene()->SetFrameBuffer(framebuffer);

	camera = new CCameraProxy();
	camera->position = { 0, 0, -1.5f };
	camera->fov = 90.f;

	camController = new CCameraController();
	camController->SetCamera(camera);

	//scene->SetPrimaryCamera(camera);

	light1 = modelEnt->AddComponent<CPointLightComponent>("Light1");
	light1->AttachTo(modelEnt->RootComponent());
	light1->SetPosition({ -1, 1, -1 });

	openMdlId = "MdlEditorOpenMdl" + FString::ToString((SizeType)this);
	saveMdlId = "MdlEditorSaveMdl" + FString::ToString((SizeType)this);

	FString id = "modelEditor_" + FString::ToString((SizeType)this);
	propertiesId = "Properties###mdlProps" + id;
	dockspaceId = "dockspace" + id;
	sceneId = "Scene##mdlScene" + id;
}

void CModelEditor::SetModel(CModelAsset* mdl)
{
	meshFiles.Clear();
	this->mdl = mdl;
	modelEnt->SetModel(mdl);
	compiler.SetModel(mdl);

	camera->position = mdl->bounds.position - camera->GetForwardVector() * FMath::Max(mdl->bounds.extents.Magnitude() * 1.5f, 1.f);

	if (mdl->File())
	{
		FKeyValue kv(mdl->File()->GetSdkPath());
		if (kv.IsOpen())
		{
			for (auto* c : kv.GetCategories())
			{
				FString name = c->GetName();
				if (auto i = name.FindLastOf('_'); i != -1)
					name.Erase(name.begin() + i, name.end());

				meshFiles.Add();

				FMeshFile& mesh = *meshFiles.last();
				mesh.name = name;
				mesh.file = *c->GetValue("file");

				FString pos = *c->GetValue("position");
				FString rot = *c->GetValue("rotation");
				FString scl = *c->GetValue("scale");

				TArray<FString> split = pos.Split(',');
				if (split.Size() > 0)
					mesh.transform.position.x = std::stof(split[0].c_str());
				if (split.Size() > 1)
					mesh.transform.position.y = std::stof(split[1].c_str());
				if (split.Size() > 2)
					mesh.transform.position.z = std::stof(split[2].c_str());

				split = rot.Split(',');
				if (split.Size() > 0)
					mesh.rotation.x = std::stof(split[0].c_str());
				if (split.Size() > 1)
					mesh.rotation.y = std::stof(split[1].c_str());
				if (split.Size() > 2)
					mesh.rotation.z = std::stof(split[2].c_str());
				mesh.transform.rotation = FQuaternion::EulerAngles(mesh.rotation);

				split = scl.Split(',');
				if (split.Size() > 0)
					mesh.transform.scale.x = std::stof(split[0].c_str());
				if (split.Size() > 1)
					mesh.transform.scale.y = std::stof(split[1].c_str());
				if (split.Size() > 2)
					mesh.transform.scale.z = std::stof(split[2].c_str());
			}
		}
	}
}

void CModelEditor::OnUpdate(double dt)
{
	int w, h;
	framebuffer->GetSize(w, h);
	if (w != viewportWidth || h != viewportHeight)
	{
		framebuffer->Resize(FMath::Max(viewportWidth, 16), FMath::Max(viewportHeight, 16));
	}

	scene->Update(dt);
	scene->GetRenderScene()->SetFrameBuffer(framebuffer);

	scene->SetPrimaryCamera(camera);
}

void CModelEditor::OnUIRender()
{
	FString id = "modelEditor_" + FString::ToString((SizeType)this);
	static FString title;
	title = "Model Editor";
	if (mdl && mdl->File())
		title += " - " + mdl->File()->Name();
	else if (mdl)
		title += " - New Model";

	title += "###" + id;
	
	static FString f;
	static FString m;
	f.Clear();
	m.Clear();

	if (ThoriumEditor::AcceptFile(openMdlId, &f) && !f.IsEmpty())
	{
		CModelAsset* m = CResourceManager::GetResource<CModelAsset>(f);
		if (m)
			SetModel(m);
	}
	if (ThoriumEditor::AcceptFile(saveMdlId, &f, &m) && !f.IsEmpty())
	{
		CResourceManager::RegisterNewResource(mdl, f, m);
		SaveMdl();
	}

	bool bOpen = true;
	ImGui::SetNextWindowSize(ImVec2(1280, 840), ImGuiCond_FirstUseEver);

	auto flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings;
	if (!bSaved)
		flags |= ImGuiWindowFlags_UnsavedDocument;

	int openPopup = 0;
	if (ImGui::Begin(title.c_str(), &bOpen, flags))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl+N"))
				{
					openPopup = 1;
				}
				if (ImGui::MenuItem("Open", "Ctrl+O"))
				{
					openPopup = 2;
				}
				if (ImGui::MenuItem("Save", "Ctrl+S"))
				{
					openPopup = 3;
				}
				if (ImGui::MenuItem("Save As", "Ctrl+Shift+S"));

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Recalculate Bounds") && mdl)
				{
					for (auto& m : mdl->GetMeshes())
						m.CalculateBounds();
					mdl->CalculateBounds();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		ImGuiID dockspace_id = ImGui::GetID(dockspaceId.c_str());
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0);

		if (!_init)
		{
			_init = true;

			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGui::DockBuilderAddNode(dockspace_id);

			ImGuiID dock1 = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.3f, nullptr, &dockspace_id);
			ImGuiID dock2 = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.7f, nullptr, &dockspace_id);

			ImGui::DockBuilderDockWindow(propertiesId.c_str(), dock1);
			ImGui::DockBuilderDockWindow(sceneId.c_str(), dock2);

			ImGui::DockBuilderFinish(dockspace_id);
		}

		if (ImGui::Begin(sceneId.c_str(), 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
		{
			auto wndSize = ImGui::GetContentRegionAvail();

			int wndX, wndY;
			gEngine->GetGameWindow()->GetWindowPos(&wndX, &wndY);

			auto cursorPos = ImGui::GetCursorScreenPos();
			viewportX = cursorPos.x - (float)wndX;
			viewportY = cursorPos.y - (float)wndY;

			DirectXFrameBuffer* fb = (DirectXFrameBuffer*)framebuffer;
			ImGui::Image(fb->view, { wndSize.x, wndSize.y });

			if (ImGui::BeginDragDropTarget())
			{
				if (auto* p = ImGui::AcceptDragDropPayload("THORIUM_ASSET_FILE"); p != nullptr)
				{
					FFile* file = *(FFile**)p->Data;
					FAssetClass* type = CResourceManager::GetResourceTypeByFile(file);
					if (type == (FAssetClass*)CModelAsset::StaticClass())
					{
						SetModel(CResourceManager::GetResource<CModelAsset>(file->Path()));
					}
				}
				ImGui::EndDragDropTarget();
			}

			camController->Update(gEngine->DeltaTime());

			viewportWidth = FMath::Max((int)wndSize.x, 32);
			viewportHeight = FMath::Max((int)wndSize.y, 32);
		}
		ImGui::End();

		if (ImGui::Begin(propertiesId.c_str(), 0, ImGuiWindowFlags_AlwaysUseWindowPadding))
		{
			ImGui::BeginDisabled(mdl == nullptr);

			ImVec2 mdlPropsSize = ImGui::GetContentRegionAvail();

			if (ImGui::Button(bCompiled ? "Compiled!" : "Needs Compiling!", ImVec2(mdlPropsSize.x, 28)))
				Compile();

			if (ImGui::BeginTable("entityComponentsEdit", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
			{
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Value");
				ImGui::TableHeadersRow();

				ImVec2 cursor = ImGui::GetCursorPos();
				ImVec2 availSize = ImGui::GetContentRegionAvail();

				bool bOpen = ImGui::TableTreeHeader("Meshes", ImGuiTreeNodeFlags_AllowOverlap);
				if (bOpen)
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
				
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.180f, 0.180f, 0.180f, 1.000f));
					if (ImGui::Button("Add##meshAdd", ImVec2(0, 24)))
					{
						bCompiled = false;
						bSaved = false;
						meshFiles.Add();
					}
					ImGui::SameLine();
					if (ImGui::Button("Clear##meshClear", ImVec2(0, 24)))
					{
						bCompiled = false;
						bSaved = false;
						meshFiles.Clear();
					}
					ImGui::PopStyleColor();

					int i = 0;
					int remove = -1;
					for (auto& mesh : meshFiles)
					{
						/*ImGui::TableNextRow();
						ImGui::TableNextColumn();*/

						bOpen = ImGui::TableTreeHeader(mesh.name.IsEmpty() ? ("New Mesh##" + FString::ToString(i)).c_str() : (mesh.name + "##" + FString::ToString(i)).c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap, true);
						ImGui::TableNextColumn();
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
						if (ImGui::Button("Remove"))
							remove = i;
						ImGui::SameLine();
						if (ImGui::Button("Reload"))
						{
							LoadMeshFile(mesh);
							bCompiled = false;
						}
						ImGui::PopStyleColor();

						if (bOpen)
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text("File");
							ImGui::TableNextColumn();

							if (ImGui::InputText(("##meshFile" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.file, ImGuiInputTextFlags_EnterReturnsTrue))
							{
								LoadMeshFile(mesh);
								bCompiled = false;
							}

							ImGui::SameLine();

							if (ImGui::Button(("Browse##browseMesh" + FString::ToString(i)).c_str()))
							{
								const char* f = "All Files (*)\0*.*\0FBX (.fbx)\0*.fbx\0Wavefront (.obj)\0*.obj\0glTF (.gltf .glb)\0*.gltf;*.glb\0\0";
								FString filter;
								filter.Resize(90);
								memcpy(filter.Data(), f, 90);

								FString file = CEngine::OpenFileDialog(filter);
								if (!file.IsEmpty())
								{
									mesh.file = file;
									LoadMeshFile(mesh);
									bCompiled = false;
								}
							}

							{
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Position");
								ImGui::TableNextColumn();

								//auto areaSize = ImGui::GetContentRegionAvail();
								//float tWidth = areaSize.x / 3 - 5.f;

								/*ImGui::SetNextItemWidth(tWidth);
								ImGui::DragFloat(("##_posInputX" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.transform.position.x, 0.1f);
								ImGui::SameLine();
								ImGui::SetNextItemWidth(tWidth);
								ImGui::DragFloat(("##_posInputY" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.transform.position.y, 0.1f);
								ImGui::SameLine();
								ImGui::SetNextItemWidth(tWidth);
								ImGui::DragFloat(("##_posInputZ" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.transform.position.z, 0.1f);*/

								ImGui::DragVector(("##_posinput" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.transform.position, 0.1f);
							}
							{
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Rotation");
								ImGui::TableNextColumn();

								/*auto areaSize = ImGui::GetContentRegionAvail();
								float tWidth = areaSize.x / 3 - 5.f;

								ImGui::SetNextItemWidth(tWidth);
								if (ImGui::DragFloat(("##_rotInputX" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.rotation.x, 0.1f))
									mesh.transform.rotation = FQuaternion::EulerAngles(mesh.rotation);
								ImGui::SameLine();
								ImGui::SetNextItemWidth(tWidth);
								if (ImGui::DragFloat(("##_rotInputY" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.rotation.y, 0.1f))
									mesh.transform.rotation = FQuaternion::EulerAngles(mesh.rotation);
								ImGui::SameLine();
								ImGui::SetNextItemWidth(tWidth);
								if (ImGui::DragFloat(("##_rotInputZ" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.rotation.z, 0.1f))
									mesh.transform.rotation = FQuaternion::EulerAngles(mesh.rotation);*/

								if (ImGui::DragVector(("##_rotinput" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.rotation, 0.1f))
									mesh.transform.rotation = FQuaternion::EulerAngles(mesh.rotation);
							}
							{
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Scale");
								ImGui::TableNextColumn();

								auto areaSize = ImGui::GetContentRegionAvail();
								float tWidth = areaSize.x / 3 - 5.f;

								/*ImGui::SetNextItemWidth(tWidth);
								ImGui::DragFloat(("##_sclInputX" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.transform.scale.x, 0.1f);
								ImGui::SameLine();
								ImGui::SetNextItemWidth(tWidth);
								ImGui::DragFloat(("##_sclInputY" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.transform.scale.y, 0.1f);
								ImGui::SameLine();
								ImGui::SetNextItemWidth(tWidth);
								ImGui::DragFloat(("##_sclInputZ" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.transform.scale.z, 0.1f);*/

								ImGui::DragVector(("##_scaleinput" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.transform.scale, 0.1f);
							}

							/*if (ImGui::TableTreeHeader(("Import##" + FString::ToString((SizeType)&mesh)).c_str(), 0, true))
							{
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Import Textures");
								ImGui::TableNextColumn();

								ImGui::Checkbox(("##importTex" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.bImportTextures);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Import Materials");
								ImGui::TableNextColumn();

								ImGui::Checkbox(("##importMats" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.bImportMaterials);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Import Animations");
								ImGui::TableNextColumn();

								ImGui::Checkbox(("##importAnims" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.bImportAnimations);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Import Tangents");
								ImGui::TableNextColumn();

								ImGui::Checkbox(("##importTan" + FString::ToString((SizeType)&mesh)).c_str(), &mesh.bImportTangents);

								ImGui::TreePop();
							}*/

							DrawMeshResources(mesh);

							ImGui::TreePop();
						}
						i++;
					}

					if (remove != -1)
						meshFiles.Erase(meshFiles.At(remove));

					ImGui::TreePop();
				}

				if (ImGui::TableTreeHeader("Materials", ImGuiTreeNodeFlags_AllowOverlap))
				{
					if (mdl)
					{
						for (auto& mat : mdl->materials)
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text(mat.name.c_str());
							ImGui::TableNextColumn();

							auto* matObj = (TObjectPtr<CAsset>*)&mat.obj;
							if (ImGui::AssetPtrWidget(("##_matObjPtr" + mat.name).c_str(), &matObj, 1, (FAssetClass*)CMaterial::StaticClass()))
								if (mat.obj)
									mat.path = mat.obj->File() ? mat.obj->File()->Path() : FString();
						}
					}

					ImGui::TreePop();
				}

				if (ImGui::TableTreeHeader("Colliders", ImGuiTreeNodeFlags_AllowOverlap))
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.180f, 0.180f, 0.180f, 1.000f));
					if (ImGui::Button("Add##colliderAdd", ImVec2(0, 24)))
					{
						bCompiled = false;
						bSaved = false;
						mdl->colliders.Add();
					}
					ImGui::SameLine();
					if (ImGui::Button("Clear##colliderClear", ImVec2(0, 24)))
					{
						bCompiled = false;
						bSaved = false;
						mdl->colliders.Clear();
					}
					ImGui::PopStyleColor();

					int remove = -1;
					for (int i = 0; i < mdl->colliders.Size(); i++)
					{
						FModelCollider& coll = mdl->colliders[i];

						bOpen = ImGui::TableTreeHeader(("Collider " + FString::ToString(i)).c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap, true);
						ImGui::TableNextColumn();
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
						if (ImGui::Button("Remove"))
							remove = i;
						ImGui::SameLine();
						ImGui::PopStyleColor();

						if (bOpen)
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();

							ImGui::Text("Type");

							constexpr static const char* colliderTypeNames[] = {
								"INVALID",
								"Box",
								"Sphere",
								"Plane (not working)",
								"Capsule",
								"Mesh",
								"Convex Mesh"
							};

							ImGui::TableNextColumn();

							bool bTypeChanged = false;

							if (ImGui::BeginCombo(("##colliderType" + FString::ToString(i)).c_str(), colliderTypeNames[(int)coll.shapeType]))
							{
								for (int i = 0; i < EShapeType::SHAPE_END - 1; i++)
								{
									if (ImGui::Selectable(colliderTypeNames[i + 1], coll.shapeType == i + 1))
									{
										coll.shapeType = (EShapeType)(i + 1);
										bTypeChanged = true;
									}
								}

								ImGui::EndCombo();
							}

							if (bTypeChanged)
							{
								coll.shape[0] = 0.f;
								coll.shape[1] = 0.f;
								coll.shape[2] = 0.f;
								coll.shape[3] = 1.f;
								coll.shape[4] = 1.f;
								coll.shape[5] = 1.f;
							}

							FVector& offset = *(FVector*)coll.shape;
							if (coll.shapeType < SHAPE_MESH)
							{
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Offset");
								ImGui::TableNextColumn();

								ImGui::DragVector("##offsetVec", &offset);

								if (coll.shapeType == SHAPE_BOX)
								{
									ImGui::TableNextRow();
									ImGui::TableNextColumn();
									ImGui::Text("Size");
									ImGui::TableNextColumn();

									FVector* size = ((FVector*)coll.shape) + 1;
									ImGui::DragVector("##sizeVec", size);

									gDebugRenderer->SetScene(scene->GetRenderScene());
									gDebugRenderer->DrawBox(FTransform(offset, FQuaternion(), *size), FColor::green.WithAlpha(0.1f), DebugDrawType_Solid | DebugDrawType_Overlay);
									gDebugRenderer->DrawBox(FTransform(offset, FQuaternion(), *size), FColor::green.WithAlpha(0.5f), DebugDrawType_Wireframe | DebugDrawType_Overlay);
									gDebugRenderer->SetScene(nullptr);
								}
								else if (coll.shapeType == SHAPE_SPHERE)
								{
									ImGui::TableNextRow();
									ImGui::TableNextColumn();
									ImGui::Text("Radius");
									ImGui::TableNextColumn();

									float* size = ((float*)coll.shape) + 3;
									ImGui::DragFloat("##sizeVec", size);
								}
								else if (coll.shapeType == SHAPE_CAPSULE)
								{
									ImGui::TableNextRow();
									ImGui::TableNextColumn();
									ImGui::Text("Radius");
									ImGui::TableNextColumn();

									float* size = ((float*)coll.shape) + 3;
									ImGui::DragFloat("##sizeVec", size);

									ImGui::TableNextRow();
									ImGui::TableNextColumn();
									ImGui::Text("Height");
									ImGui::TableNextColumn();

									float* height = ((float*)coll.shape) + 4;
									ImGui::DragFloat("##sizeVec", height);
								}
							}

							ImGui::TreePop();
						}
					}
					if (remove != -1)
						mdl->colliders.Erase(mdl->colliders.At(remove));

					ImGui::TreePop();
				}

				if (ImGui::TableTreeHeader("Body Groups", ImGuiTreeNodeFlags_AllowOverlap))
				{


					ImGui::TreePop();
				}

				if (ImGui::TableTreeHeader("LOD Groups", ImGuiTreeNodeFlags_AllowOverlap))
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.180f, 0.180f, 0.180f, 1.000f));
					if (ImGui::Button("Add##lodAdd", ImVec2(0, 24)))
					{
						bSaved = false;
						mdl->numLODs++;
						if (mdl->numLODs > 6)
							mdl->numLODs = 6;
						else
							mdl->LODs[mdl->numLODs - 1] = FLODGroup();
					}
					ImGui::SameLine();
					if (ImGui::Button("Clear##lodClear", ImVec2(0, 24)))
					{
						bSaved = false;
						mdl->numLODs = 0;
					}
					ImGui::PopStyleColor();

					for (int i = 0; i < mdl->numLODs; i++)
					{
						auto& lod = mdl->LODs[i];
						if (ImGui::TableTreeHeader(("LOD " + FString::ToString(i)).c_str(), 0, true))
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text("Distance Bias");
							ImGui::TableNextColumn();
							ImGui::DragFloat(("##lodBias" + FString::ToString(i)).c_str(), &mdl->LODs[i].distanceBias, 0.1f);

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text("Meshes");
							ImGui::TableNextColumn();

							const char* preview = lod.meshIndices.Size() == 0 ? "none" : (lod.meshIndices.Size() > 1 ? "Multiple" : mdl->meshes[lod.meshIndices[0]].meshName.c_str());
							if (ImGui::BeginCombo(("##lodMeshes" + FString::ToString(i)).c_str(), preview))
							{
								for (int m = 0; m < mdl->meshes.Size(); m++)
								{
									bool bSelected = false;
									for (auto& ii : lod.meshIndices)
										if (ii == m)
											bSelected = true;

									if (ImGui::Selectable((mdl->meshes[m].meshName + "##_comboMesh" + FString::ToString(m)).c_str(), bSelected))
									{
										if (bSelected)
											lod.meshIndices.Erase(lod.meshIndices.Find(m));
										else
											lod.meshIndices.Add(m);
									}
								}

								ImGui::EndCombo();
							}

							ImGui::TreePop();
						}
					}

					ImGui::TreePop();
				}

				if (ImGui::TableTreeHeader("Bones"))
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Count");
					ImGui::TableNextColumn();
					ImGui::Text("%d", (int)mdl->skeleton.bones.Size());

					int highlight = -1;

					for (SizeType i = 0; i < mdl->skeleton.bones.Size(); i++)
					{
						auto& b = mdl->skeleton.bones[i];
						if (ImGui::TableTreeHeader(b.name.c_str()))
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text("Parent");
							ImGui::TableNextColumn();
							ImGui::Text("%s", b.parent != -1 ? mdl->skeleton.bones[b.parent].name.c_str() : "none");

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text("Position");
							ImGui::TableNextColumn();
							ImGui::BeginDisabled();
							ImGui::DragVector("##_bonePosition", &b.position);
							ImGui::EndDisabled();

							ImGui::TreePop();
						}

						if (ImGui::IsItemHovered())
							highlight = i;
					}

					RenderSkeleton(highlight);
					ImGui::TreePop();
				}

				ImGui::EndTable();
			}

			ImGui::EndDisabled();
		}
		ImGui::End();
	}
	ImGui::End();

	// Update MouseRay
	mouseRay = FRay::MouseToRay(camera, gEngine->InputManager()->GetMousePos() - FVector2(viewportX, viewportY), { (float)viewportWidth, (float)viewportHeight });
	mouseRay.direction = mouseRay.direction.Normalize();

	if (openPopup == 1)
	{
		if (mdl && !bSaved)
		{
			// Save first
			saveCallback = &CModelEditor::OnSaveNewModel;
			ImGui::OpenPopup("Continue without saving?##_MDLEDITCLOSE");
		}
		else
		{
			/*SetModel(CreateObject<CModelAsset>());
			bSaved = false;*/
			OnSaveNewModel(1);
		}
	}
	else if (openPopup == 2)
	{
		if (!bSaved && mdl)
		{
			saveCallback = &CModelEditor::OnSaveOpenModel;
			ImGui::OpenPopup("Continue without saving?##_MDLEDITCLOSE");
		}
		else
			ThoriumEditor::OpenFile(openMdlId, (FAssetClass*)CModelAsset::StaticClass());
	}
	else if (openPopup == 3)
	{
		SaveMdl();
	}

	if (!bOpen)
	{
		bExit = true;

		if (!bSaved && mdl)
		{
			saveCallback = &CModelEditor::OnSaveExit;
			ImGui::OpenPopup("Continue without saving?##_MDLEDITCLOSE");
		}
		else
			gEditorEngine()->PollRemoveLayer(this);
	}

	int savePopupResult = -1;

	if (ImGui::BeginPopupModal("Continue without saving?##_MDLEDITCLOSE"))
	{
		ImGui::Text("are you sure you want to continue without saving?");

		if (ImGui::Button("Save"))
		{
			ImGui::CloseCurrentPopup();
			SaveMdl();
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

void CModelEditor::OnDetach()
{
	scene->Delete();
	delete framebuffer;
	delete camera;
}

void LoadMeshFile(FMeshFile& m)
{
	unsigned int flags = aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_OptimizeMeshes | aiProcess_FlipUVs | aiProcess_PopulateArmatureData;

	//if (!m.bImportTangents)
		flags |= aiProcess_CalcTangentSpace;

	m.scene = m.importer.ReadFile(m.file.c_str(), flags);
	if (!m.scene)
		return;

	m.name = m.file;
	
	if (auto i = m.name.FindLastOf("/\\"); i != -1)
		m.name.Erase(m.name.begin(), m.name.begin() + i + 1);
}

void CModelEditor::Compile()
{
	//TArray<FMaterial> oldMats = mdl->materials;

	//mdl->ClearMeshData();
	//mdl->meshes.Clear();
	//mdl->materials.Clear();
	//mdl->skeleton.bones.Clear();

	//SizeType meshesOffset = 0;
	//SizeType materialsOffset = 0;
	//SizeType boneOffset = 0;

	//for (auto& file : meshFiles)
	//{
	//	if (!file.scene)
	//		LoadMeshFile(file);

	//	if (file.scene)
	//	{
	//		auto* scene = file.scene;
	//		auto* root = scene->mRootNode;

	//		for (uint i = 0; i < scene->mNumMaterials; i++)
	//		{
	//			aiMaterial* sMat = scene->mMaterials[i];

	//			FMaterial mat;
	//			mat.name = sMat->GetName().C_Str();

	//			mdl->materials.Add(mat);
	//		}

	//		/*for (uint i = 0; i < scene->mNumSkeletons; i++)
	//		{
	//			aiSkeleton* skeleton = scene->mSkeletons[i];

	//			for (int ii = 0; ii < skeleton->mNumBones; ii++)
	//			{
	//				aiSkeletonBone* bone = skeleton->mBones[ii];
	//				aiMatrix4x4& mat = bone->mLocalMatrix;

	//				aiVector3D scale;
	//				aiVector3D pos;
	//				aiQuaternion rot;

	//				mat.Decompose(scale, rot, pos);

	//				FBone b;
	//				b.name = bone->mNode ? bone->mNode->mName.C_Str() : "BONE";
	//				b.position = { pos.x, pos.y, pos.z };
	//				b.rotation = { rot.x, rot.y, rot.z, rot.w };

	//				b.parent = bone->mParent == -1 ? -1 : bone->mParent + (int)boneOffset;

	//				mdl->skeleton.bones.Add(b);
	//			}
	//		}*/

	//		TArray<TPair<int, aiBone*>> bones;

	//		CompileNode(file, scene, root, meshesOffset, materialsOffset, bones);

	//		for (auto& b : bones)
	//		{
	//			FBone newBone;
	//			newBone.name = b.Value->mName.C_Str();

	//			aiVector3D scale;
	//			aiVector3D pos;
	//			aiQuaternion rot;

	//			b.Value->mNode->mTransformation.Decompose(scale, rot, pos);
	//			newBone.position = { pos.x, pos.y, pos.z };
	//			newBone.rotation = { rot.x, rot.y, rot.z, rot.w };

	//			auto& mesh = mdl->meshes[b.Key];

	//			for (int i = 0; i < b.Value->mNumWeights; i++)
	//			{
	//				if (!mesh.vertexData)
	//					continue;

	//				auto& weight = b.Value->mWeights[i];
	//				if (weight.mVertexId >= mesh.numVertexData)
	//					continue;

	//				FVertex& vertex = mesh.vertexData[weight.mVertexId];

	//				for (int x = 0; x < 4; x++)
	//				{
	//					if (vertex.bones[x] == -1)
	//					{
	//						vertex.bones[x] = (int)mdl->skeleton.bones.Size();
	//						vertex.boneInfluence[x] = weight.mWeight;
	//						break;
	//					}
	//				}
	//			}

	//			mdl->skeleton.bones.Add(newBone);
	//		}

	//		// Resolve bone parents
	//		for (int i = 0; i < bones.Size(); i++)
	//		{
	//			aiNode* parent = bones[i].Value->mNode->mParent;
	//			mdl->skeleton.bones[i].parent = mdl->GetBoneIndex(parent->mName.C_Str());
	//		}

	//		for (int i = 0; i < mdl->meshes.Size(); i++)
	//		{
	//			if (mdl->meshes[i].meshName.IsEmpty())
	//				mdl->meshes[i].meshName = "Mesh " + FString::ToString(i);
	//		}

	//		materialsOffset = mdl->materials.Size();
	//		meshesOffset = mdl->meshes.Size();
	//		boneOffset = mdl->skeleton.bones.Size();
	//	}
	//}

	//for (auto& mat : mdl->materials)
	//{
	//	for (auto& m : oldMats)
	//	{
	//		if (mat.name == m.name)
	//		{
	//			mat.obj = m.obj;
	//			mat.path = m.path;
	//			break;
	//		}
	//	}
	//}

	//mdl->CalculateBounds();
	//mdl->UpdateBoneMatrices();

	if (compiler.Compile(mdl, meshFiles.Data(), meshFiles.Size()))
	{
		bCompiled = true;
		modelEnt->SetModel(mdl);
	}
	else
		bCompiled = false;
}

aiMatrix4x4 GetNodeWorldTransform(aiNode* node)
{
	if (node->mParent)
		return node->mTransformation * GetNodeWorldTransform(node->mParent);

	return node->mTransformation;
}

void CModelCompiler::CompileNode(FMeshFile& file, const aiScene* scene, aiNode* node, SizeType& meshOffset, SizeType& matOffset, TArray<TPair<int, aiBone*>>& outBones)
{
	//for (uint i = 0; i < node->mNumMeshes; i++)
	//{
	//	aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

	//	for (uint ii = 0; ii < mesh->mNumBones; ii++)
	//	{
	//		aiBone* bone = mesh->mBones[ii];


	//	}
	//}

	for (uint i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		aiMatrix4x4 transform = GetNodeWorldTransform(node);

		FMatrix mat = file.transform.ToMatrix();
		transform = (*(aiMatrix4x4*)&mat) * transform;

		FMesh fmesh;
		fmesh.meshName = mesh->mName.C_Str();
		fmesh.materialIndex = mesh->mMaterialIndex + (int)matOffset;
		
		if (mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE)
			fmesh.topologyType = FMesh::TOPOLOGY_TRIANGLES;
		else if (mesh->mPrimitiveTypes == aiPrimitiveType_LINE)
			fmesh.topologyType = FMesh::TOPOLOGY_LINES;
		else if (mesh->mPrimitiveTypes == aiPrimitiveType_POINT)
			fmesh.topologyType = FMesh::TOPOLOGY_POINTS;

		TArray<FVertex> vertices;
		TArray<uint> indices;

		for (uint i = 0; i < mesh->mNumVertices; i++)
		{
			FVertex v;
			v.bones[0] = -1;
			v.bones[1] = -1;
			v.bones[2] = -1;
			v.bones[3] = -1;
			v.boneInfluence[0] = 0.f;
			v.boneInfluence[1] = 0.f;
			v.boneInfluence[2] = 0.f;
			v.boneInfluence[3] = 0.f;

			auto vPos = mesh->mVertices[i];
			auto vNormal = mesh->mNormals ? mesh->mNormals[i] : aiVector3D(0, 1, 0);
			auto vTangent = mesh->mTangents ? mesh->mTangents[i] : aiVector3D(1, 0, 0);

			vPos *= transform;
			vNormal *= transform;
			vTangent *= transform;

			v.position = *(FVector*)&vPos;
			if (mesh->mNormals)
				v.normal = *(FVector*)&vNormal;
			if (mesh->mTangents)
				v.tangent = *(FVector*)&vTangent;
			if (mesh->GetNumColorChannels() > 0)
				v.color = *(FVector*)&mesh->mColors[0][i];

			if (mesh->GetNumUVChannels() > 0)
			{
				v.uv1[0] = mesh->mTextureCoords[0][i].x;
				v.uv1[1] = mesh->mTextureCoords[0][i].y;
			}
			if (mesh->GetNumUVChannels() > 1)
			{
				v.uv2[0] = mesh->mTextureCoords[1][i].x;
				v.uv2[1] = mesh->mTextureCoords[1][i].y;
			}

			vertices.Add(v);
		}

		for (uint i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace& face = mesh->mFaces[i];

			if (face.mNumIndices == 3)
			{
				indices.Add(face.mIndices[0]);
				indices.Add(face.mIndices[2]);
				indices.Add(face.mIndices[1]);
			}
			else if (face.mNumIndices == 2)
			{
				indices.Add(face.mIndices[0]);
				indices.Add(face.mIndices[1]);
			}
			else
				indices.Add(face.mIndices[0]);
		}

		for (uint i = 0; i < mesh->mNumBones; i++)
		{
			auto* b = mesh->mBones[i];
			outBones.Add({ (int)mdl->meshes.Size(), b });

			//SizeType bIndex = mdl->GetBoneIndex(b->mName.C_Str());
			//if (bIndex == -1)
			//{
			//	
			//	continue;
			//}

			//for (uint ii = 0; ii < b->mNumWeights; ii++)
			//{
			//	auto& weight = b->mWeights[ii];
			//	for (int x = 0; x < 4; x++)
			//	{
			//		if (vertices[weight.mVertexId].bones[x] == -1)
			//		{
			//			vertices[weight.mVertexId].bones[x] = (int)bIndex;
			//			vertices[weight.mVertexId].boneInfluence[x] = weight.mWeight;
			//			break;
			//		}
			//	}
			//}
		}

		fmesh.vertexData = (FVertex*)malloc(vertices.Size() * sizeof(FVertex));
		fmesh.numVertexData = vertices.Size();

		memcpy(fmesh.vertexData, vertices.Data(), vertices.Size() * sizeof(FVertex));

		fmesh.indexData = (uint*)malloc(indices.Size() * sizeof(uint));
		fmesh.numIndexData = indices.Size();

		memcpy(fmesh.indexData, indices.Data(), indices.Size() * sizeof(uint));

		fmesh.numVertices = vertices.Size();
		fmesh.numIndices = indices.Size();

		//fmesh.vertexBuffer = gRenderer->CreateVertexBuffer(vertices);
		//fmesh.indexBuffer = gRenderer->CreateIndexBuffer(indices);

		fmesh.CalculateBounds();

		mdl->meshes.Add(fmesh);
	}

	for (int i = 0; i < node->mNumChildren; i++)
		CompileNode(file, scene, node->mChildren[i], meshOffset, matOffset, outBones);
}

void CModelEditor::DrawMeshResources(FMeshFile& m)
{
	const aiScene* scene = m.scene;

	if (!scene)
		return;

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::Text("Meshes");
	ImGui::TableNextColumn();
	ImGui::Text("%d", scene->mNumMeshes);

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::Text("Materials");
	ImGui::TableNextColumn();
	ImGui::Text("%d", scene->mNumMaterials);

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::Text("Textures");
	ImGui::TableNextColumn();
	ImGui::Text("%d", scene->mNumTextures);

	if (scene->mRootNode)
		DrawAiNode(scene, scene->mRootNode);

	if (scene->mNumAnimations > 0)
		DrawAnimations(scene);
}

void CModelEditor::DrawAiNode(const aiScene* scene, aiNode* node)
{
	if (ImGui::TableTreeHeader(node->mName.C_Str(), 0, true))
	{
		if (node->mNumMeshes)
		{
			if (ImGui::TableTreeHeader("Meshes", 0, true))
			{
				for (uint i = 0; i < node->mNumMeshes; i++)
				{
					aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
					if (ImGui::TableTreeHeader(mesh->mName.C_Str(), 0, true))
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Vertices");
						ImGui::TableNextColumn();
						ImGui::Text("%d", mesh->mNumVertices);

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Faces");
						ImGui::TableNextColumn();
						ImGui::Text("%d", mesh->mNumFaces);

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Has Normals");
						ImGui::TableNextColumn();
						ImGui::Text(mesh->mNormals ? "true" : "false");

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Has Tangents");
						ImGui::TableNextColumn();
						ImGui::Text(mesh->mTangents ? "true" : "false");

						ImGui::TreePop();
					}
					if (mesh->mNumBones > 0 && ImGui::TableTreeHeader(("Bones##mesh" + FString::ToString(i)).c_str(), 0, true))
					{
						for (int b = 0; b < mesh->mNumBones; b++)
						{
							aiBone* bone = mesh->mBones[b];

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text(bone->mName.C_Str());
							ImGui::TableNextColumn();
							ImGui::Text("Weights: %d", bone->mNumWeights);
						}

						ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}
		}

		if (node->mNumChildren > 0)
		{
			if (ImGui::TableTreeHeader("Children", 0, true))
			{
				for (uint i = 0; i < node->mNumChildren; i++)
					DrawAiNode(scene, node->mChildren[i]);

				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}
}

void CModelEditor::DrawAnimations(const aiScene* scene)
{
	if (ImGui::TableTreeHeader("Animations", 0, true))
	{
		for (int i = 0; i < scene->mNumAnimations; i++)
		{
			aiAnimation* anim = scene->mAnimations[i];

			if (ImGui::TableTreeHeader(anim->mName.C_Str(), 0, true))
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (ImGui::Button("Export"))
				{
					// TODO: export animation
				}
				
				ImGui::SameLine(); ImGui::TextDisabled("(?)");
				if (ImGui::BeginItemTooltip())
				{
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted("Animations are stored in a seperate file\nto use animations it must be exported from the model and into its own file.");
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Duration");
				ImGui::TableNextColumn();
				ImGui::Text("%0.3fs", (float)anim->mDuration / (float)anim->mTicksPerSecond);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Frame Rate");
				ImGui::TableNextColumn();
				ImGui::Text("%0.1f", (float)anim->mTicksPerSecond);

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}
}

void CModelEditor::OnSaveNewModel(int r)
{
	if (r != 0)
	{
		SetModel(CreateObject<CModelAsset>());
	}
}

void CModelEditor::OnSaveExit(int r)
{
	if (r == 0)
		bExit = false;
	else
		gEditorEngine()->PollRemoveLayer(this);
}

void CModelEditor::OnSaveOpenModel(int r)
{
	if (r != 0)
		ThoriumEditor::OpenFile(openMdlId, (FAssetClass*)CModelAsset::StaticClass());
}

void CModelEditor::SaveMdl()
{
	if (!mdl)
		return;

	if (!mdl->File())
	{
		ThoriumEditor::SaveFile(saveMdlId, (FAssetClass*)CMaterial::StaticClass());
		return;
	}

	compiler.SaveModel(meshFiles.Data(), meshFiles.Size());
	bSaved = true;
}

void CModelEditor::Revert()
{
	// ??
	bSaved = true;
}

void CModelEditor::RenderSkeleton(int highlight)
{
	gDebugRenderer->SetScene(scene->GetRenderScene());
	
	for (int i = 0; i < mdl->skeleton.bones.Size(); i++)
	{
		FBone& bone = mdl->skeleton.bones[i];
		FBone* parent = bone.parent == -1 ? nullptr : &mdl->skeleton.bones[bone.parent];

		FTransform boneTransform = mdl->GetBoneModelTransform(i);

		if (FMath::RayBox(FBounds(boneTransform.position, FVector(0.05f)), FQuaternion(), mouseRay))
			highlight = i;
		
		gDebugRenderer->DrawBox(FTransform(boneTransform.position, FQuaternion(), FVector(0.1f)), highlight == i ? FColor::lime : FColor::blue.WithAlpha(0.5f), DebugDrawType_Solid | DebugDrawType_Overlay);

		if (!parent || bone.parent == 0)
			continue;

		FTransform parentTransform = mdl->GetBoneModelTransform(bone.parent);

		gDebugRenderer->DrawLine(boneTransform.position, parentTransform.position, FColor::orange, 0, true);
	}

	gDebugRenderer->SetScene(nullptr);
}

bool CModelCompiler::Compile(CModelAsset* _mdl, FMeshFile* meshFiles, int numMeshFiles, const FModelCompileSettings& settings)
{
	mdl = _mdl;

	TArray<FMaterial> oldMats = mdl->materials;

	mdl->ClearMeshData();
	mdl->meshes.Clear();
	//mdl->ClearMeshes();
	mdl->materials.Clear();
	mdl->skeleton.bones.Clear();

	SizeType meshesOffset = 0;
	SizeType materialsOffset = 0;
	SizeType boneOffset = 0;

	for (int i = 0; i < numMeshFiles; i++)
	{
		auto& file = meshFiles[i];

		if (!file.scene)
			LoadMeshFile(file);

		if (file.scene)
		{
			auto* scene = file.scene;
			auto* root = scene->mRootNode;

			for (uint i = 0; i < scene->mNumMaterials; i++)
			{
				aiMaterial* sMat = scene->mMaterials[i];

				FMaterial mat;
				mat.name = sMat->GetName().C_Str();

				if (settings.bCreateMaterials)
				{
					
				}

				mdl->materials.Add(mat);
			}

			/*for (uint i = 0; i < scene->mNumSkeletons; i++)
			{
				aiSkeleton* skeleton = scene->mSkeletons[i];

				for (int ii = 0; ii < skeleton->mNumBones; ii++)
				{
					aiSkeletonBone* bone = skeleton->mBones[ii];
					aiMatrix4x4& mat = bone->mLocalMatrix;

					aiVector3D scale;
					aiVector3D pos;
					aiQuaternion rot;

					mat.Decompose(scale, rot, pos);

					FBone b;
					b.name = bone->mNode ? bone->mNode->mName.C_Str() : "BONE";
					b.position = { pos.x, pos.y, pos.z };
					b.rotation = { rot.x, rot.y, rot.z, rot.w };

					b.parent = bone->mParent == -1 ? -1 : bone->mParent + (int)boneOffset;

					mdl->skeleton.bones.Add(b);
				}
			}*/

			TArray<TPair<int, aiBone*>> bones;

			CompileNode(file, scene, root, meshesOffset, materialsOffset, bones);

			for (auto& b : bones)
			{
				FBone newBone;
				newBone.name = b.Value->mName.C_Str();

				aiVector3D scale;
				aiVector3D pos;
				aiQuaternion rot;

				b.Value->mNode->mTransformation.Decompose(scale, rot, pos);
				newBone.position = { pos.x, pos.y, pos.z };
				newBone.rotation = { rot.x, rot.y, rot.z, rot.w };

				auto& mesh = mdl->meshes[b.Key];

				for (int i = 0; i < b.Value->mNumWeights; i++)
				{
					if (!mesh.vertexData)
						continue;

					auto& weight = b.Value->mWeights[i];
					if (weight.mVertexId >= mesh.numVertexData)
						continue;

					FVertex& vertex = mesh.vertexData[weight.mVertexId];

					for (int x = 0; x < 4; x++)
					{
						if (vertex.bones[x] == -1)
						{
							vertex.bones[x] = (int)mdl->skeleton.bones.Size();
							vertex.boneInfluence[x] = weight.mWeight;
							break;
						}
					}
				}

				mdl->skeleton.bones.Add(newBone);
			}

			// Resolve bone parents
			for (int i = 0; i < bones.Size(); i++)
			{
				aiNode* parent = bones[i].Value->mNode->mParent;
				mdl->skeleton.bones[i].parent = mdl->GetBoneIndex(parent->mName.C_Str());
			}

			for (int i = 0; i < mdl->meshes.Size(); i++)
			{
				if (mdl->meshes[i].meshName.IsEmpty())
					mdl->meshes[i].meshName = "Mesh " + FString::ToString(i);

				// Create render buffers for meshes
				struct ImSorry
				{
					void* data;
					SizeType size;
					SizeType capacity;
				};
				ImSorry vertexData{ mdl->meshes[i].vertexData, mdl->meshes[i].numVertexData, mdl->meshes[i].numVertexData };
				mdl->meshes[i].vertexBuffer = gRenderer->CreateVertexBuffer(*(TArray<FVertex>*)&vertexData);

				ImSorry indexData{ mdl->meshes[i].indexData, mdl->meshes[i].numIndexData, mdl->meshes[i].numIndexData };
				mdl->meshes[i].indexBuffer = gRenderer->CreateIndexBuffer(*(TArray<uint>*)&indexData);
			}

			mdl->_SetLod(0);

			materialsOffset = mdl->materials.Size();
			meshesOffset = mdl->meshes.Size();
			boneOffset = mdl->skeleton.bones.Size();
		}
		else
			return false;
	}

	for (auto& mat : mdl->materials)
	{
		for (auto& m : oldMats)
		{
			if (mat.name == m.name)
			{
				mat.obj = m.obj;
				mat.path = m.path;
				break;
			}
		}
	}

	mdl->CalculateBounds();
	mdl->UpdateBoneMatrices();

	return true;
}

bool CModelCompiler::GenerateLODGroups(FString suffix /*= "_LOD"*/)
{
	// Meshes without suffix
	TArray<FMesh*> unkownMeshes;

	int meshesAdded = 0;
	int meshIndex = 0;
	for (auto& mesh : mdl->meshes)
	{
		meshIndex++;
		SizeType i = mesh.meshName.Find(suffix);
		if (i == -1)
		{
			unkownMeshes.Add(&mesh);
			continue;
		}

		FString index = mesh.meshName;
		index.Erase(index.begin(), index.begin() + i + suffix.Size());

		int lodIndex = -1;
		try
		{
			lodIndex = FMath::Clamp(index.ToInt(), 0, 5);
		}
		catch (std::exception& e) {}

		if (lodIndex == -1)
		{
			unkownMeshes.Add(&mesh);
			continue;
		}

		mdl->numLODs = lodIndex + 1;
		mdl->LODs[lodIndex].meshIndices.Add(meshIndex - 1);
		meshesAdded++;
	}

	if (meshesAdded > 0)
	{
		FBounds bounds = mdl->GetBounds();
		for (int i = 0; i < mdl->numLODs; i++)
		{
			mdl->LODs[i].distanceBias = bounds.Size().Magnitude() * 2 * i;
		}
	}

	return meshesAdded > 0;
}

bool CModelCompiler::GenerateConvexCollision()
{
	return false;
}

void CModelCompiler::SaveModel(FMeshFile* meshFiles, int numMeshFiles)
{
	FKeyValue kv(mdl->File()->GetSdkPath());
	for (int i = 0; i < numMeshFiles; i++)
	{
		auto* cat = kv.GetCategory(meshFiles[i].name + "_" + FString::ToString(i), true);

		cat->SetValue("file", meshFiles[i].file);

		FVector& pos = meshFiles[i].transform.position;
		FVector& rot = meshFiles[i].rotation;
		FVector& scl = meshFiles[i].transform.scale;

		cat->SetValue("position", (std::to_string(pos.x) + "," + std::to_string(pos.y) + "," + std::to_string(pos.z)).c_str());
		cat->SetValue("rotation", (std::to_string(rot.x) + "," + std::to_string(rot.y) + "," + std::to_string(rot.z)).c_str());
		cat->SetValue("scale", (std::to_string(scl.x) + "," + std::to_string(scl.y) + "," + std::to_string(scl.z)).c_str());
	}
	kv.Save();

	mdl->Save();
}

bool CModelCompiler::ImportAnimations(FMeshFile* meshFiles, int numMeshFiles, const FAnimationImportSettings& settings)
{
	return false;
}
