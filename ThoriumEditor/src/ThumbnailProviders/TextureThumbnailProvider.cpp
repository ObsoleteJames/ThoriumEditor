
#include "AssetThumbnail.h"
#include "Assets/TextureAsset.h"
#include "EditorConfig.h"

#include "Rendering/Renderer.h"
#include "Rendering/RenderScene.h"
#include "Rendering/GraphicsInterface.h"
#include "Rendering/Shader.h"

class TextureThumbnailGen : public IAssetThumbnailGenerator
{
public:
	TextureThumbnailGen(CTexture* t, FAssetThumbnail* th) : tex(t), thumbnail(th)
	{
		t->Load(0);
	}

	bool IsReady() override
	{
		if (tex->IsLodLoaded(0))
			return true;
		return false;
	}

	void Execute() override
	{
		int size = evThumbnailMaxSize.GetValue().AsInt();
		auto* fb = gGHI->CreateFrameBuffer(size, size, TEXTURE_FORMAT_RGBA8_UNORM);

		auto* shScreenPlane = CShaderSource::GetShaderSource("ScreenPlaneVS");
		auto* shBlit = CShaderSource::GetShaderSource("Editor_TextureThumbnail");
		shBlit->LoadShaderObjects();

		gGHI->SetViewport(0, 0, (float)size, (float)size);
		gGHI->SetFrameBuffer(fb);
		gGHI->SetShaderResource(tex->GetTextureObject(), 1);
		gGHI->SetVsShader(shScreenPlane->GetShader(ShaderType_Vertex));
		gGHI->SetPsShader(shBlit->GetShader(ShaderType_Fragment));
		gGHI->SetBlendMode(EBlendMode::BLEND_ADDITIVE);

		fb->Clear(0, 0, 0, 0);

		FMesh mesh;
		mesh.numVertices = 3;
		gGHI->DrawMesh(&mesh);

		auto* source = gGHI->CreateTexture2D({ TextureType_2D, (uint)size, (uint)size, 1, 1, TEXTURE_FORMAT_RGBA8_UNORM, THTX_FILTER_LINEAR, TH_BUFFER_FLAGS_CPU_READ, nullptr });
		gGHI->CopyResource(fb, source);

		delete fb;

		FMappedResource imgData;
		source->Map(&imgData, TH_RESOURCE_MAP_READ);

		QImage image((const uchar*)imgData.data, size, size, imgData.rowPitch, QImage::Format_RGBA8888);
		QImage copy = image.copy();

		source->Unmap();

		delete source;

		thumbnail->image = QPixmap::fromImage(copy);
		thumbnail->state = EThumbnailState::Loaded;
		bIsDone = true;
	}

private:
	FAssetThumbnail* thumbnail;
	TObjectPtr<CTexture> tex;
};

static FAssetThumbnailProvider texProvider("CTexture", CTexture::StaticClass(), [](CAsset* asset, FAssetThumbnail* thumb) { 
	return new TextureThumbnailGen((CTexture*)asset, thumb); 
});
