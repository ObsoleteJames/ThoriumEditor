
#include "Gizmo.h"
#include "Rendering/RenderScene.h"
#include "Rendering/DebugRenderer.h"
#include "Game/Input/InputManager.h"

constexpr float ArrowLength = 0.25f;
constexpr float ArrowShaftRadius = 0.005f;
constexpr float ArrowHeadLength = 0.06f;
constexpr float ArrowHeadRadius = 0.03f;
constexpr float PickRadius = 0.015f;
constexpr float PlaneSize = 0.15f;
constexpr float PlaneAlpha = 0.3f;

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

void FGizmo::Manipulate(float view[16], float projection[16], bool bLocal, FVector& pos, FQuaternion& rot, FVector& scale, FTransform* outDelta)
{
	if (!renderScene)
		return;

	Render(pos, bLocal ? rot : FQuaternion());
}

void FGizmo::Render(const FVector& pos, const FQuaternion& rot)
{
	CDebugRenderer* debugRenderer = renderScene->DebugRenderer();
	if (!debugRenderer)
		return;

	FVector xAxis = rot.Rotate(FVector(1.f, 0.f, 0.f));
	DrawArrow(debugRenderer, pos, xAxis, FColor::red.WithAlpha(0.75f));

	FVector yAxis = rot.Rotate(FVector(0.f, 1.f, 0.f));
	DrawArrow(debugRenderer, pos, yAxis, FColor::green.WithAlpha(0.75f));

	FVector zAxis = rot.Rotate(FVector(0.f, 0.f, -1.f));
	DrawArrow(debugRenderer, pos, zAxis, FColor::blue.WithAlpha(0.75f));

	debugRenderer->DrawSphere(pos, PickRadius, FColor::white.WithAlpha(0.75f), DebugDrawType_Solid | DebugDrawType_Overlay);
}
