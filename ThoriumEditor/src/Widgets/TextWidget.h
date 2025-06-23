#pragma once

#include "WWidget.h"
#include "Rendering/Texture.h"

#include "TextWidget.generated.h"

CLASS()
class WTextWidget : public WWidget
{
	GENERATED_BODY()

public:
	WTextWidget();

public:
	void Render(BLContext* ctx) override;

	ITexture2D* tex;
};
