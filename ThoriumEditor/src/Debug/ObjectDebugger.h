#pragma once

#include "Layer.h"
#include "EditorCore.h"
#include "Object/Object.h"

class CObjectDebugger : public CLayer
{
public:
	void OnUIRender() override;

private:
	TObjectPtr<CObject> selected;
};
