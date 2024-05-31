#pragma once

#include "Rendering/RenderProxies.h"

enum ECameraControlMode
{
	CCM_FreeCam,
	CCM_Orbit
};

class CCameraController
{
public:
	void Update(double dt);
	
	void SetCamera(CCameraProxy* cam);

	inline FVector GetMoveVector() const { return FVector((float)(moveLeft + -moveRight), (float)(moveDown + -moveUp), (float)(-moveForward + moveBack)); }

public:
	ECameraControlMode mode = CCM_FreeCam;
	int cameraSpeed = 4;
	int minCamSpeed = 1;
	int maxCamSpeed = 10;

private:
	CCameraProxy* camera;
	FVector orbitPos;
	//ImVec2 clickPos;
	bool bMoving;

	float camPitch = 0.f;
	float camYaw = 0.f;

	float curSpeed = 0.f;

	int8 moveForward : 1;
	int8 moveBack : 1;
	int8 moveLeft : 1;
	int8 moveRight : 1;
	int8 moveUp : 1;
	int8 moveDown : 1;

	bool bMouseLeft : 1;
	bool bMouseRight : 1;
	bool bMouseMiddle : 1;
};
