#pragma once

#include "Layer.h"
#include "EngineCore.h"
#include "Object/Object.h"
#include "Resources/ModelAsset.h"

#include "assimp/Importer.hpp"

class CWorld;
class CModelEntity;
class CRenderScene;
class CCameraProxy;
class CPointLightComponent;
class CCameraController;

class IFrameBuffer;
class IDepthBuffer;

struct aiNode;
struct aiScene;
struct aiBone;

struct FMeshFile
{
	FString file;
	FString name;
	FTransform transform;
	FVector rotation; // fuck quaterions man.

	Assimp::Importer importer;
	const aiScene* scene = nullptr;
};

void LoadMeshFile(FMeshFile& m);

enum EColliderImportType
{
	COLLIDER_OFF,
	COLLIDER_SIMPLE,
	COLLIDER_COMPLEX
};

struct FModelCompileSettings
{
	bool bCreateMaterials = false;
	bool bImportTextures = false;

	FString materialsOut;

	// if true, will use all meshes as colliders, does NOT generate convex colliders.
	EColliderImportType bUseMeshesAsCollision = COLLIDER_COMPLEX;
};

struct FAnimationImportSettings
{
	FString outputFolder;


};

class CModelCompiler
{
public:
	CModelCompiler() = default;

	inline void SetModel(CModelAsset* model) { mdl = model; }

	bool Compile(CModelAsset* mdl, FMeshFile* meshFiles, int numMeshFiles, const FModelCompileSettings& settings = FModelCompileSettings());

	/* 
	 *	Generates LOD groups based on names of meshes.
	 *	meshes without a suffix will default to LOD 0.
	 *	returns true if any LOD groups were made. 
	 */
	bool GenerateLODGroups(FString suffix = "_LOD");

	bool GenerateConvexCollision();

	void SaveModel(FMeshFile* meshFiles, int numMeshFiles);

	bool ImportAnimations(FMeshFile* meshFiles, int numMeshFiles, const FAnimationImportSettings& settings);

private:
	void CompileNode(FMeshFile& file, const aiScene* scene, aiNode* node, SizeType& meshOffset, SizeType& matOffset, TArray<TPair<int, aiBone*>>& outBones);

private:
	CModelAsset* mdl;
};

class CModelEditor : public CLayer
{
	typedef void(CModelEditor::* funcSaveCallback)(int result);

public:
	CModelEditor();

	void SetModel(CModelAsset* mdl);
	inline CModelAsset* GetModel() const { return mdl; }

protected:
	void OnUpdate(double dt) override;
	void OnUIRender() override;

	void OnDetach() override;

	void Compile();
	//void CompileNode(FMeshFile& file, const aiScene* scene, aiNode* node, SizeType& meshOffset, SizeType& matOffset, TArray<TPair<int, aiBone*>>& outBones);
	//void CompileArmatureNode(FMeshFile& file, const aiScene* scene, aiNode* node, SizeType& meshOffset, SizeType& matOffset, SizeType& boneOffset);

	void DrawMeshResources(FMeshFile& m);
	void DrawAiNode(const aiScene* scene, aiNode* node);
	void DrawAnimations(const aiScene* scene);

	void OnSaveNewModel(int r);
	void OnSaveExit(int r);
	void OnSaveOpenModel(int r);

	void SaveMdl();
	void Revert();

	void RenderSkeleton(int highlight = -1);
	
private:
	bool _init = false;

	// window IDs
	FString propertiesId;
	FString dockspaceId;
	FString sceneId;

private:
	bool bSaved = true;
	bool bCompiled = true;
	bool bExit = false;

	TObjectPtr<CModelAsset> mdl;
	TObjectPtr<CModelEntity> modelEnt;
	TObjectPtr<CWorld> scene;

	CModelCompiler compiler;

	// List of imported meshes
	TArray<FMeshFile> meshFiles;

	CCameraProxy* camera;
	CCameraController* camController;

	CPointLightComponent* light1;

	FRay mouseRay;
	
	IFrameBuffer* framebuffer;

	FString openMdlId;
	FString saveMdlId;

	funcSaveCallback saveCallback = nullptr;

	//float sizeL = 420;
	//float sizeR;

	int viewportWidth = 1280, viewportHeight = 720;
	float viewportX = 0.f, viewportY = 0.f;
};
