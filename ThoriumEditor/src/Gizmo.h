#pragma once

#include "Math/Vectors.h"
#include "Math/Transform.h"
#include "Object/Object.h"

class CRenderScene;

enum EGizmoMode
{
	Gizmo_Select = 0,
	Gizmo_Translate,
	Gizmo_Rotate,
	Gizmo_Scale,
	Gizmo_Bounds
};

class FGizmo : public CObject
{
public:
	FGizmo() = default; 

	void Manipulate(float view[16], float projection[16], bool bLocal, FVector& pos, FQuaternion& rot, FVector& scale, FTransform* outDelta = nullptr);

private:
	void Render(const FVector& pos, const FQuaternion& rot);

public:
	int mode = Gizmo_Select;
	CRenderScene* renderScene = nullptr;

private:
	int hoverIndex = 0;
	bool bDragging = false;
	
};
