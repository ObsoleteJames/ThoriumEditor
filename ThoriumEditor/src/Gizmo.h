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

enum EGizmoPivot
{
	Pivot_Center = 0, // center of object's positions.
	Pivot_BoundingBox, // combined bounding box center.
	Pivot_Active, // active object position.
};

enum EGizmoAxis
{
	Axis_None = 0,
	Axis_X,
	Axis_Y,
	Axis_Z,
	Axis_XY,
	Axis_XZ,
	Axis_YZ,
	Axis_XYZ // center sphere
};

class FGizmo : public CObject
{
public:
	FGizmo() = default; 

	// returns true if the gizmo was manipulated
	bool Manipulate(FVector& pos, FQuaternion& rot, FVector& scale, const FRay& mouseRay, uint mouseBtns, FTransform* outDelta = nullptr);

	// Set the snapping vector, for rotation this must be the euler angle in radians.
	inline void SetSnap(const FVector& v) { bEnableSnap = true; snap = v; }
	inline void ClearSnap() { bEnableSnap = false; }
	inline const FVector& GetSnap() const { return snap; }

private:
	void Render(const FVector& pos, const FQuaternion& rot);

	bool DoTranslate(FVector& pos, FQuaternion& rot, FVector& scale, const FRay& mouseRay, uint mouseBtns, FTransform* outDelta);
	bool DoRotate(FVector& pos, FQuaternion& rot, FVector& scale, const FRay& mouseRay, uint mouseBtns, FTransform* outDelta);

private:
	int dragAxis = Axis_None;
	FVector dragStartPos = FVector::zero;
	FVector dragStartRayOrigin = FVector::zero;
	FVector dragStartRayDir = FVector::zero;
	FVector dragOffset;

	FQuaternion dragStartRot;

	FVector GetConstrainedMovement(const FVector& rayOrigin, const FVector& rayDir, const FVector& axisOrigin, const FVector& axisDir);

public:
	int mode = Gizmo_Select;
	CRenderScene* renderScene = nullptr;

	bool bGlobal = false;
	EGizmoPivot pivotMode = Pivot_Center;

	bool bEnableSnap;
	FVector snap;

	int hoverAxis = Axis_None;
	bool bDragging = false;
	
};
