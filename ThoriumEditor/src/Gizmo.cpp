
#include "Gizmo.h"
#include "Rendering/RenderScene.h"
#include "Rendering/DebugRenderer.h"

void FGizmo::Manipulate(float view[16], float projection[16], bool bLocal, FVector& pos, FQuaternion& rot, FVector& scale, FTransform* outDelta)
{

	Render(pos, bLocal ? rot : FQuaternion());
}

void FGizmo::Render(const FVector& pos, const FQuaternion& rot)
{
	renderScene->DebugRenderer()->DrawLine(pos, pos + rot.Rotate(FVector(1.f, 0.f, 0.f))  * 0.25f, FColor::red);
	renderScene->DebugRenderer()->DrawLine(pos, pos + rot.Rotate(FVector(0.f, 1.f, 0.f))  * 0.25f, FColor::green);
	renderScene->DebugRenderer()->DrawLine(pos, pos + rot.Rotate(FVector(0.f, 0.f, -1.f)) * 0.25f, FColor::blue);
}
