
#include "CameraController.h"
#include "ImGui/imgui.h"

float GetCameraSpeed(int index)
{
	float speed = 0.5f;
	speed *= (float)index;
	speed *= speed;
	return speed;
}

void CCameraController::Update(double dt)
{
	bool bHovered = ImGui::IsWindowFocused() || ImGui::IsWindowHovered();
	bMouseLeft = bHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left);
	bMouseRight = bHovered && ImGui::IsMouseDown(ImGuiMouseButton_Right);
	bMouseMiddle = bHovered && ImGui::IsMouseDown(ImGuiMouseButton_Middle);

	moveForward = ImGui::IsKeyDown(ImGuiKey_W);
	moveBack = ImGui::IsKeyDown(ImGuiKey_S);
	moveLeft = ImGui::IsKeyDown(ImGuiKey_A);
	moveRight = ImGui::IsKeyDown(ImGuiKey_D);

	moveUp = ImGui::IsKeyDown(ImGuiKey_E);
	moveDown = ImGui::IsKeyDown(ImGuiKey_Q);

	if (bMouseRight && mode == CCM_FreeCam)
	{
		camPitch = FMath::Clamp(camPitch + (ImGui::GetIO().MouseDelta.y / 5.f), -90.f, 90.f);
		camYaw += ImGui::GetIO().MouseDelta.x / 5.f;
		camera->rotation = FQuaternion::EulerAngles(FVector(camPitch, camYaw, 0.f).Radians());

		FVector move = GetMoveVector();
		float verticalMove = (float)(moveDown + -moveUp);

		if (move.Magnitude() != 0.f)
		{
			float targetSpeed = GetCameraSpeed(cameraSpeed);
			if (curSpeed < targetSpeed)
				curSpeed += (5.f * (float)cameraSpeed * (float)dt);
			else
				curSpeed = targetSpeed;

			FVector pos = camera->position;
			pos += camera->GetForwardVector() * move.z * curSpeed * (float)dt;
			pos += camera->GetRightVector() * move.x * curSpeed * (float)dt;
			pos += FVector(0, move.y, 0) * curSpeed * (float)dt;
			camera->position = pos;
		}
		else
			curSpeed = 0.f;
	}
}

void CCameraController::SetCamera(CCameraProxy* cam)
{
	camera = cam;

	FVector euler = camera->rotation.ToEuler().Degrees();
	camPitch = euler.x;
	camYaw = euler.y;
}
