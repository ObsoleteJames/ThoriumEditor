
#include "TextWidget.h"
#include "Rendering/GraphicsInterface.h"
#include "Console.h"

//#include "blend2d.h"
//
//BLFontFace face;
//BLFont font;

WTextWidget::WTextWidget()
{
	//tex = gGHI->CreateTexture2D(nullptr, 240, 160, TEXTURE_FORMAT_RGBA8_UNORM, THTX_FILTER_LINEAR);

	//FFile* fontFile = CFileSystem::FindFile("fonts/trebucbd.ttf");

	//BLResult r = face.createFromFile(fontFile->FullPath().c_str());
	//if (r != BL_SUCCESS)
	//	CONSOLE_LogError("WTextWidget", "Oop!");

	//r = font.createFromFace(face, 16);
}

void WTextWidget::Render(BLContext*)
{
	//BLImage img(240, 160, BL_FORMAT_PRGB32);
	//
	//BLContext ctx(img);
	//
	//ctx.clearAll();

	//BLGradient grad(BLLinearGradientValues(0, 0, 0, 30));
	//grad.addStop(0.01,		BLRgba32(0xFFE55300));
	//grad.addStop(0.0333,	BLRgba32(0xFFFF9A42));
	//grad.addStop(0.2333,	BLRgba32(0xFFDE5800));
	//grad.addStop(0.5333,	BLRgba32(0xFFDE5800));
	//grad.addStop(0.8,		BLRgba32(0xFFFA6C01));
	//grad.addStop(0.9,		BLRgba32(0xFFFD6701));
	//grad.addStop(1.0,		BLRgba32(0xFFD54601));

	//ctx.setFillStyle(grad);
	//ctx.fillRect(0, 0, 240, 30);
	//
	//ctx.fillUtf8Text(BLPoint(7, 21), font, "Hellorld!", -1, BLRgba32(0xFF000000));
	//ctx.fillUtf8Text(BLPoint(6, 20), font, "Hellorld!", -1, BLRgba32(0xFFFFFFFF));
	//
	//ctx.end();

	//BLImageData d;
	//img.getData(&d);
	//tex->UpdateData(d.pixelData, 0);
}
