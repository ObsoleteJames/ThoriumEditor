#pragma once

#include "Layer.h"
#include "EngineCore.h"
#include "Object/Object.h"
#include "Resources/TextureAsset.h"

class CMaterial;
class CModelEntity;
class CWorld;
class CCameraProxy;

class IFrameBuffer;
class IDepthBuffer;

class CTextureViewer : public CLayer
{
public:
	CTextureViewer();

	void SetTexture(CTexture* tex);
	inline CTexture* GetTexture() const { return tex; }

protected:
	void OnUpdate(double dt) override;
	void OnUIRender();

	void OnDetach();

	// Reimport the texture
	void UpdateTex();

private:
	bool bWantsToClose = false;
	bool bSaved = true;

	FString texSourceFile;
	FTextureImportSettings reimportSettings;

	TObjectPtr<CTexture> tex;
	//TObjectPtr<CMaterial> matUnlit;
	//TObjectPtr<CModelEntity> modelEnt;

	//TObjectPtr<CWorld> scene;

	//CCameraProxy* camera;

	//IFrameBuffer* framebuffer;
	//IDepthBuffer* depthbuffer;

	float scale = 1.f;

	FString openTexId;
	FString saveTexId;

	float sizeL = 300;
	float sizeR;

	int viewportWidth = 1280, viewportHeight = 720;
	float viewportX = 0.f, viewportY = 0.f;
};
