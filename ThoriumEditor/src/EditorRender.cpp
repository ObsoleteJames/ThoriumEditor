
#include "EditorEngine.h"
#include "Rendering/GraphicsInterface.h"
#include "Rendering/RenderScene.h"
#include "Assets/Material.h"
#include "Game/Entity.h"
#include "Game/Components/PrimitiveComponent.h"

//static bool bEditorRenderInit = 0;

//static CShaderSource* shaderSelectOverlay;
//static IShaderBuffer* objectBuffer;
//static IShaderBuffer* sceneBuffer;

void CEditorEngine::InitEditorRender()
{
	//bEditorRenderInit = true;

	shaderSelectOverlay = CShaderSource::GetShaderSource("Editor_SelectOverlay");
	if (shaderSelectOverlay)
	{
		shaderSelectOverlay->LoadShaderObjects();
		shaderSelectOverlay->MakeIndestructible();
	}

	objectBuffer = gGHI->CreateBuffer({ TH_BUFFER_TYPE_SHADER_BUFFER, sizeof(FObjectInfoBuffer), nullptr, 0, TH_BUFFER_FLAGS_CPU_WRITE });
	objectBuffer->MakeIndestructible();
	sceneBuffer = gGHI->CreateBuffer({ TH_BUFFER_TYPE_SHADER_BUFFER, sizeof(FSceneInfoBuffer), nullptr, 0, TH_BUFFER_FLAGS_CPU_WRITE });
	sceneBuffer->MakeIndestructible();
}

void CEditorEngine::DoEditorRender()
{
	//if (!bEditorRenderInit)
	//	_InitEditorRender();

	if (!bSelectionOverlay)
		return;

	CRenderScene* scene = gWorld->GetRenderScene();
	CCameraProxy* camera = viewportCams[0];

	IFrameBuffer* renderTarget = camera->renderTarget ? camera->renderTarget : scene->frameBuffer;
	if (!renderTarget)
		return;

	int viewWidth, viewHeight;
	renderTarget->GetSize(viewWidth, viewHeight);

	if (scene->depth)
		scene->depth->Clear();

	static TArray<TPair<CPrimitiveProxy*, FMeshBuilder>> meshes;
	meshes.Clear();

	//static TArray<CPrimitiveComponent*> comps;

	for (TObjectPtr<CObject> object : selectedObjects)
	{
		auto ent = Cast<CEntity>(object);
		if (!ent)
			continue;

		auto& comps = ent->GetAllComponents();

		for (auto c : comps)
		{
			auto comp = Cast<CPrimitiveComponent>(c.second);
			if (!comp)
				continue;

			auto* proxy = comp->PrimitiveProxy();

			if (!proxy->IsVisible())
				continue;

			meshes.Add({ proxy, FMeshBuilder() });
			FMeshBuilder& mesh = meshes.last()->Value;

			proxy->GetStaticMeshes(mesh);
			proxy->GetSkinnedMeshes(mesh);
		}
	}

	FMatrix camMatrix = camera->projection * camera->view;
	FSceneInfoBuffer sceneInfo{ camMatrix, camera->view, camera->projection,
		camMatrix.Inverse(), camera->view.Inverse(), camera->projection.Inverse(),
		camera->position, 0u, camera->GetForwardVector(), 0u, scene->GetTime(), 1.0f, 1.6f, 0,
		FVector2((float)viewWidth, (float)viewHeight) / FVector2((float)scene->GetFrameBufferWidth(), (float)scene->GetFrameBufferHeight()),
		FVector2(viewWidth, viewHeight)
	};
	sceneBuffer->Update(sizeof(FSceneInfoBuffer), &sceneInfo);

	gGHI->SetViewport(0.f, 0.f, (float)viewWidth, (float)viewHeight);
	gGHI->SetFrameBuffer(renderTarget);

	gGHI->SetShaderBuffer(sceneBuffer, 1);
	gGHI->SetShaderBuffer(objectBuffer, 3);

	gGHI->SetShaderResource(scene->preTranslucentBuff, 3);
	gGHI->SetShaderResource(scene->depthTex, 2);

	for (auto& obj : meshes)
	{
		for (auto& mesh : obj.Value.GetMeshes())
		{
			FObjectInfoBuffer objectInfo;
			objectInfo.transform = mesh.transform;
			objectInfo.position = obj.Key->GetPosition();
			memcpy(objectInfo.skeletonMatrices, mesh.skeletonMatrices.Data(), FMath::Min((int)mesh.skeletonMatrices.Size(), 48) * sizeof(FMatrix));
			objectBuffer->Update(sizeof(FObjectInfoBuffer), &objectInfo);

			//IShader* _shader = mesh.mat->GetVsShader(ShaderType_DeferredPass);
			//if (!_shader)
			//	_shader = mesh.mat->GetVsShader(ShaderType_ForwardPass);
			//gGHI->SetVsShader(_shader);
			gGHI->SetVsShader(shaderSelectOverlay->GetShader(mesh.mesh.bSkinnedMesh ? ShaderType_VertexSkinned : ShaderType_Vertex));
			gGHI->SetPsShader(shaderSelectOverlay->GetShader(ShaderType_Fragment));
			
			gGHI->DrawMesh(&mesh);
		}
	}
}
