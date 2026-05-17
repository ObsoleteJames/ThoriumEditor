
#include "Gizmo.h"
#include "Rendering/RenderScene.h"
#include "Rendering/DebugRenderer.h"
#include "Game/Input/InputManager.h"

#include <QWidget>

constexpr float ArrowLength = 0.25f;
constexpr float ArrowShaftRadius = 0.005f;
constexpr float ArrowHeadLength = 0.06f;
constexpr float ArrowHeadRadius = 0.03f;
constexpr float PickRadius = 0.03f;
constexpr float PlaneSize = 0.15f;
constexpr float PlaneAlpha = 0.3f;

constexpr float RotationSize = 0.25f;
constexpr float RotateHandleThickness = 0.03f;

void DrawArrow(CDebugRenderer* renderer, const FVector& origin, const FVector& direction, const FColor& color)
{
	if (!renderer)
		return;

	FVector normalizedDir = direction;
	normalizedDir.Normalize();
	FVector arrowEnd = origin + normalizedDir * ArrowLength;
	FVector arrowHeadBase = arrowEnd - normalizedDir * ArrowHeadLength;
	FVector shaftCenter = origin + normalizedDir * (ArrowLength - ArrowHeadLength) * 0.6f;

	FQuaternion rot = FQuaternion::LookRotation(normalizedDir, FVector(0.f, 1.f, 0.f)) * FQuaternion::EulerAngles(FVector(90, 0, 0).Radians());

	renderer->DrawCylinder(
		shaftCenter,
		rot,
		ArrowLength - ArrowHeadLength - 0.02f,
		ArrowShaftRadius,
		color,
		DebugDrawType_Solid | DebugDrawType_Overlay
	);

	renderer->DrawCone(
		arrowEnd,
		rot,
		ArrowHeadLength,
		ArrowHeadRadius,
		color,
		DebugDrawType_Solid | DebugDrawType_Overlay
	);
}

float RayPlaneIntersection(const FVector& rayOrigin, const FVector& rayDir, const FVector& planeOrigin, const FVector& planeNormal)
{
	float denom = FVector::Dot(rayDir, planeNormal);
	if (FMath::Abs(denom) > 1e-6f)
	{
		return FVector::Dot(planeOrigin - rayOrigin, planeNormal) / denom;
	}
	return -1.f; // No intersection or ray is parallel to plane
}

FVector FGizmo::GetConstrainedMovement(const FVector& rayOrigin, const FVector& rayDir, const FVector& axisOrigin, const FVector& axisNormal)
{
	float t = RayPlaneIntersection(rayOrigin, rayDir, axisOrigin, axisNormal);
	float len = fabs(t);
	FVector intersectPos = rayOrigin + rayDir * len;

	return intersectPos;
}

bool FGizmo::Manipulate(FVector& pos, FQuaternion& rot, FVector& scale, const FRay& mouseRay, uint mouseBtns, FTransform* outDelta)
{
	if (!renderScene)
		return false;

	bool r = false;
	FQuaternion gizmoRot = bGlobal ? FQuaternion() : rot;

	if (mode == Gizmo_Translate)
		r = DoTranslate(pos, gizmoRot, scale, mouseRay, mouseBtns, outDelta);
	if (mode == Gizmo_Rotate)
		r = DoRotate(pos, gizmoRot, scale, mouseRay, mouseBtns, outDelta);

	Render(pos, gizmoRot);
	return r;
}

bool FGizmo::DoTranslate(FVector& pos, FQuaternion& rot, FVector& scale, const FRay& mouseRay, uint mouseBtns, FTransform* outDelta)
{
	bool r = false;

	FVector xAxis = rot.Rotate(FVector(1.f, 0.f, 0.f)).Normalize();
	FVector yAxis = rot.Rotate(FVector(0.f, 1.f, 0.f)).Normalize();
	FVector zAxis = rot.Rotate(FVector(0.f, 0.f, 1.f)).Normalize();

	const FVector axis[] = { xAxis, yAxis, zAxis };
	const FVector axisPlane[] = {
		FVector::Cross(xAxis, yAxis).Normalize(),
		FVector::Cross(xAxis, yAxis).Normalize(),
		FVector::Cross(xAxis, zAxis).Normalize()
	};

	bool bHover;
	double dist;

	hoverAxis = Axis_None;
	FMath::RayCylinderIntersection(pos + xAxis * (ArrowLength - ArrowHeadLength) * 0.7f, xAxis, PickRadius, ArrowLength, mouseRay.origin, mouseRay.direction, bHover, dist);
	if (bHover)
		hoverAxis = Axis_X;

	FMath::RayCylinderIntersection(pos + yAxis * (ArrowLength - ArrowHeadLength) * 0.7f, yAxis, PickRadius, ArrowLength, mouseRay.origin, mouseRay.direction, bHover, dist);
	if (bHover)
		hoverAxis = Axis_Y;

	FMath::RayCylinderIntersection(pos + zAxis * (ArrowLength - ArrowHeadLength) * 0.7f, zAxis, PickRadius, ArrowLength, mouseRay.origin, mouseRay.direction, bHover, dist);
	if (bHover)
		hoverAxis = Axis_Z;

	if (FMath::RaySphere(pos, 0.015f, mouseRay))
		hoverAxis = Axis_XYZ;

	// Handle mouse down - start dragging
	if (mouseBtns & Qt::LeftButton && !bDragging && hoverAxis != Axis_None)
	{
		bDragging = true;
		dragAxis = hoverAxis;
		dragStartRayOrigin = mouseRay.origin;
		dragStartRayDir = mouseRay.direction;
		dragStartPos = pos;

		// Calculate drag offset to prevent jumping
		FVector planeNormal;
		if (dragAxis < Axis_XY)
			planeNormal = axisPlane[dragAxis - 1];
		else if (dragAxis == Axis_XYZ)
			planeNormal = (dragStartRayOrigin - dragStartPos).Normalize();

		FVector rayIntersect = GetConstrainedMovement(mouseRay.origin, mouseRay.direction, dragStartPos, planeNormal);
		dragOffset = dragStartPos - rayIntersect;
	}

	// Handle dragging
	if (bDragging && (mouseBtns & Qt::LeftButton))
	{
		FVector delta = FVector::zero;

		FVector planeNormal;
		if (dragAxis < Axis_XY)
			planeNormal = axisPlane[dragAxis - 1];

		FVector newPos = GetConstrainedMovement(mouseRay.origin, mouseRay.direction, dragStartPos, planeNormal);
		newPos += dragOffset;
		delta = newPos - dragStartPos;

		// Single axis constraint
		if (dragAxis < Axis_XY)
		{
			const float distOnAxis = FVector::Dot(delta, axis[dragAxis - 1]);
			delta = axis[dragAxis - 1] * distOnAxis;
		}
		else if (dragAxis == Axis_XYZ)
		{
			// Free movement on a plane perpendicular to view direction
			FVector viewDir = (dragStartRayOrigin - dragStartPos).Normalize();
			FVector dragPlaneNormal = viewDir;
			float planeDistance = FVector::Dot(dragStartPos, dragPlaneNormal);

			float rayT = (planeDistance - FVector::Dot(mouseRay.origin, dragPlaneNormal)) / FVector::Dot(mouseRay.direction, dragPlaneNormal);
			if (rayT > 0.f)
			{
				FVector currentPos = mouseRay.origin + mouseRay.direction * rayT;
				currentPos += dragOffset;
				delta = currentPos - dragStartPos;
			}
		}

		pos = dragStartPos + delta;

		if (outDelta)
		{
			outDelta->position = delta;
			outDelta->rotation = FQuaternion();
			outDelta->scale = FVector(1.f, 1.f, 1.f);
		}

		r = true;
	}
	else if (!(mouseBtns & Qt::LeftButton) && bDragging)
	{
		bDragging = false;
		dragAxis = Axis_None;
		dragOffset = FVector::zero;
	}
	return r;
}

bool FGizmo::DoRotate(FVector& pos, FQuaternion& rot, FVector& scale, const FRay& mouseRay, uint mouseBtns, FTransform* outDelta)
{
	if (!bDragging)
		hoverAxis = Axis_None;

	bool r = false;
	if (bDragging)
		rot = dragStartRot;

	FVector xAxis = rot.Rotate(FVector(1.f, 0.f, 0.f)).Normalize();
	FVector yAxis = rot.Rotate(FVector(0.f, 1.f, 0.f)).Normalize();
	FVector zAxis = rot.Rotate(FVector(0.f, 0.f, 1.f)).Normalize();

	const FVector axis[] = { xAxis, yAxis, zAxis };
	const FVector axisPlane[] = {
		FVector::Cross(zAxis, yAxis).Normalize(),
		FVector::Cross(xAxis, zAxis).Normalize(),
		FVector::Cross(xAxis, yAxis).Normalize()
	};

	FVector intersectPos{};
	if (!bDragging)
	{
		for (int i = 0; i < 3; i++)
		{
			float t = RayPlaneIntersection(mouseRay.origin, mouseRay.direction, pos, axisPlane[i]);
			float len = fabs(t);
			FVector intersect = mouseRay.origin + mouseRay.direction * len;

			float dist = FVector::Distance(intersect, pos);
			float halfThick = RotateHandleThickness * 0.5f;
			bool bHit = dist > (RotationSize - halfThick) && dist < (RotationSize + halfThick);

			if (bHit)
			{
				hoverAxis = Axis_X + i;
				intersectPos = intersect;
			}
		}
	}

	if (mouseBtns & Qt::LeftButton && !bDragging && hoverAxis != Axis_None)
	{
		bDragging = true;
		dragAxis = hoverAxis;
		dragStartPos = intersectPos;
		dragStartRot = rot;
	}
	if (bDragging && (mouseBtns & Qt::LeftButton))
	{
		float t = RayPlaneIntersection(mouseRay.origin, mouseRay.direction, pos, axisPlane[dragAxis - 1]);
		float len = fabs(t);
		FVector intersect = mouseRay.origin + mouseRay.direction * len;

		FVector rotAxis = axis[dragAxis - 1];

		const FVector v1 = (dragStartPos - pos).Normalize();
		const FVector v2 = (intersect - pos).Normalize();

		float theta = FVector::Dot(v1, v2);
		theta = FMath::Acos(FMath::Clamp(theta, -1.f, 1.f));

		if (FVector::Dot(rotAxis, FVector::Cross(v1, v2)) < 0.f)
			theta = -theta;

		if (bEnableSnap)
		{
			float snapV = ((float*)&snap)[dragAxis - 1];
			theta = std::round(theta / snapV) * snapV;
		}

		float hT = theta * 0.5f; // half theta
		float thetaSin = FMath::Sin(hT);

		FQuaternion deltaQ = FQuaternion(rotAxis.x * thetaSin, rotAxis.y * thetaSin, rotAxis.z * thetaSin, FMath::Cos(hT));

		rot = (deltaQ * dragStartRot).Normalized();

		if (outDelta)
		{
			outDelta->position = FVector();
			outDelta->rotation = deltaQ;
			outDelta->scale = FVector(1.f, 1.f, 1.f);
		}

		r = true;
	}
	else if (!(mouseBtns & Qt::LeftButton) && bDragging)
	{
		bDragging = false;
		dragAxis = Axis_None;
	}

	return r;
}

#define GIZMO_ROT_LINES 32

void FGizmo::Render(const FVector& pos, const FQuaternion& rot)
{
	CDebugRenderer* debugRenderer = renderScene->DebugRenderer();
	if (!debugRenderer)
		return;

	FColor hoverColor = FColor::yellow.WithAlpha(0.75f);

	if (mode == Gizmo_Translate)
	{
		FVector xAxis = rot.Rotate(FVector(1.f, 0.f, 0.f));
		DrawArrow(debugRenderer, pos, xAxis, hoverAxis == 1 ? hoverColor : FColor::red.WithAlpha(0.75f));

		FVector yAxis = rot.Rotate(FVector(0.f, 1.f, 0.f));
		DrawArrow(debugRenderer, pos, yAxis, hoverAxis == 2 ? hoverColor : FColor::green.WithAlpha(0.75f));

		FVector zAxis = rot.Rotate(FVector(0.f, 0.f, 1.f));
		DrawArrow(debugRenderer, pos, zAxis, hoverAxis == 3 ? hoverColor : FColor::blue.WithAlpha(0.75f));

		debugRenderer->DrawSphere(pos, 0.015f, hoverAxis == Axis_XYZ ? hoverColor : FColor::white.WithAlpha(0.75f), DebugDrawType_Solid | DebugDrawType_Overlay);
	}
	if (mode == Gizmo_Rotate)
	{
		for (int i = 0; i < GIZMO_ROT_LINES; i++)
		{
			float fI = float(i) / float(GIZMO_ROT_LINES) * FMath::Pi() * 2;
			float fI2 = float(i + 1) / float(GIZMO_ROT_LINES) * FMath::Pi() * 2;

			float s1 = FMath::Sin(fI);
			float c1 = FMath::Cos(fI);
			float s2 = FMath::Sin(fI2);
			float c2 = FMath::Cos(fI2);

			FVector x1{ 0, s1, c1};
			FVector x2{ 0, s2, c2 };

			FVector y1{ s1, 0, c1 };
			FVector y2{ s2, 0, c2 };

			FVector z1{ s1, c1, 0 };
			FVector z2{ s2, c2, 0 };

			x1 = rot.Rotate(x1);
			x2 = rot.Rotate(x2);

			y1 = rot.Rotate(y1);
			y2 = rot.Rotate(y2);

			z1 = rot.Rotate(z1);
			z2 = rot.Rotate(z2);

			debugRenderer->DrawLine(x1 * RotationSize + pos, x2 * RotationSize + pos, hoverAxis == 1 ? hoverColor : FColor::red, 0, true);
			debugRenderer->DrawLine(y1 * RotationSize + pos, y2 * RotationSize + pos, hoverAxis == 2 ? hoverColor : FColor::green, 0, true);
			debugRenderer->DrawLine(z1 * RotationSize + pos, z2 * RotationSize + pos, hoverAxis == 3 ? hoverColor : FColor::blue, 0, true);
		}
	}
}
