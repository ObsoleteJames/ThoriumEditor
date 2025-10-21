
#include "ViewportWidget.h"
#include "EngineThread.h"
#include "Engine.h"
#include "Game/World.h"
#include "Game/Entity.h"

#include "Rendering/RenderProxies.h"
#include "Rendering/RenderScene.h"
#include "EditorEngine.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QComboBox>

float GetCameraSpeed(int index)
{
	float speed = 0.5f;
	speed *= (float)index;
	speed *= speed;
	return speed;
}

float GetCameraOrthoZoom(int index)
{
	float zoom = 0.1f;
	return zoom * ((index * index) / 1.5f);
}

CViewportWidget::CViewportWidget(QWidget* parent, bool bCreateCam) : CRenderWidget(parent)
{
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);
	bMouseLeft = 0;
	bMouseMiddle = 0;
	bMouseRight = 0;

	moveForward = 0;
	moveBack = 0;
	moveLeft = 0;
	moveRight = 0;
	moveUp = 0;
	moveDown = 0;

	if (bCreateCam)
	{
		SetCamera(new CCameraProxy());
		bOwnsCamera = true;
		camera->bEnabled = true;
	}

	//comboViewMode = new QComboBox(this);
	//comboViewMode->addItem("Lit");
	//comboViewMode->addItem("Unlit");
	//comboViewMode->addItem("Wireframe");
	//comboViewMode->addItem("Diffuse Colour");
	//comboViewMode->addItem("Normals");
	//comboViewMode->addItem("Reflections only");
	//comboViewMode->setCurrentIndex(0);

	connect(gEngineThread, &CEngineThread::onUpdate, this, &CViewportWidget::OnUpdate);
}

CViewportWidget::~CViewportWidget()
{
}

void CViewportWidget::SetCamera(CCameraProxy* cam)
{
	if (bOwnsCamera)
		delete camera;

	camera = cam;
	//if (GetSwapChain())
	//	camera->renderTarget = GetSwapChain()->GetFrameBuffer();
	
	FVector euler = camera->rotation.ToEuler().Degrees();
	cameraPitch = euler.x;
	cameraYaw = euler.y;

	if (camera->bOrthographic)
		mode = ECameraControlMode::Ortho;

	bOwnsCamera = false;
}

void CViewportWidget::OnUpdate()
{
	double dt = gEngine->DeltaTime();

	if (!camera)
		return;

	camera->bEnabled = isVisible();

	if (camera->bOrthographic)
		camera->fov = GetCameraOrthoZoom(orthoZoom);

	//FVector euler = camera->rotation.ToEuler().Degrees();
	//cameraPitch = euler.x;
	//cameraYaw = euler.y;

	if (bRotateCam && mode == ECameraControlMode::FreeMode)
	{
		cameraPitch = FMath::Clamp(cameraPitch + mouseDeltaY, -90.f, 90.f);
		cameraYaw += mouseDeltaX;
		camera->rotation = FQuaternion::EulerAngles(FVector(cameraPitch, cameraYaw, 0.f).Radians());

		FVector move = GetMoveVector();
		float verticalMove = moveDown + -moveUp;

		if (move.Magnitude() != 0.0f || verticalMove != 0.f)
		{
			if (curSpeed < GetCameraSpeed(cameraSpeed))
				curSpeed += (5.f * (float)cameraSpeed) * (float)dt;
			else
				curSpeed = GetCameraSpeed(cameraSpeed);

			FVector pos = camera->position;
			pos += camera->GetForwardVector() * move.y * curSpeed * dt;
			pos += camera->GetRightVector() * move.x * curSpeed * dt;
			pos += FVector(0, verticalMove, 0) * curSpeed * dt;
			camera->position = pos;
		}
		else
			curSpeed = 0.f;
	}
	if (bMouseLeft && mode == ECameraControlMode::Orbit)
	{
		cameraPitch = FMath::Clamp(cameraPitch + mouseDeltaY, -90.f, 90.f);
		cameraYaw += mouseDeltaX;
		camera->rotation = FQuaternion::EulerAngles(FVector(cameraPitch, cameraYaw, 0.f).Radians());
	}
	if (bMouseMiddle)
	{
		if (mode == ECameraControlMode::FreeMode)
		{
			FVector pos = camera->position;
			pos += camera->GetUpVector() * mouseDeltaY * dt;
			pos += camera->GetRightVector() * -mouseDeltaX * dt;
			camera->position = pos;
		}
		else if (mode == ECameraControlMode::Orbit)
		{
			orbitPos += camera->GetUpVector() * mouseDeltaY * dt;
			orbitPos += camera->GetRightVector() * -mouseDeltaX * dt;
		}
		else if (mode == ECameraControlMode::Ortho)
		{
			FVector pos = camera->position;
			pos += camera->GetUpVector() * mouseDeltaY * camera->fov * dt;
			pos += camera->GetRightVector() * -mouseDeltaX * camera->fov * dt;
			camera->position = pos;
		}
	}

	if (mode == ECameraControlMode::Orbit)
		camera->position = (-camera->GetForwardVector() * (15 - cameraSpeed)) + orbitPos;

	mouseDeltaY = 0;
	mouseDeltaX = 0;
}

void CViewportWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (bMouseRight && !bRotateCam && mouseClickPos != event->globalPos())
	{
		QCursor cursor(Qt::BlankCursor);
		QApplication::setOverrideCursor(cursor);

		bRotateCam = true;
	}

	mouseDeltaX = ((float)event->x() - (float)prevMouseX) * 0.25f;
	mouseDeltaY = ((float)event->y() - (float)prevMouseY) * 0.25f;
	prevMouseX = event->x();
	prevMouseY = event->y();
	CRenderWidget::mouseMoveEvent(event);
}

void CViewportWidget::mousePressEvent(QMouseEvent* event)
{
	CRenderWidget::mousePressEvent(event);
	if (bMouseLeft || bMouseMiddle || bMouseRight)
		return;

	mouseClickPos = event->globalPos();
	switch (event->button())
	{
	case Qt::LeftButton:
		bMouseLeft = true;
		break;
	case Qt::RightButton:
	{
		if (mode == ECameraControlMode::FreeMode)
		{
			mouseClickPos = event->globalPos();
		}
		bMouseRight = true;
	}
	break;
	case Qt::MiddleButton:
		bMouseMiddle = true;
		break;
	}
}

void CViewportWidget::mouseReleaseEvent(QMouseEvent* event)
{
	CRenderWidget::mouseReleaseEvent(event);
	switch (event->button())
	{
	case Qt::LeftButton:
	{
		if (mouseClickPos == event->globalPos())
			DoMousePick(event->localPos());

		bMouseLeft = false;
	}
		break;
	case Qt::RightButton:
		if (bMouseRight)
		{
			bMouseRight = false;

			// do right click on entity
		}
		if (bRotateCam)
		{
			if (mode == ECameraControlMode::FreeMode)
			{
				QApplication::restoreOverrideCursor();
				QCursor::setPos(mouseClickPos);
			}

			bRotateCam = false;
		}
		break;
	case Qt::MiddleButton:
		bMouseMiddle = false;
		break;
	}
}

void CViewportWidget::keyPressEvent(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_W:
		moveForward = 1;
		break;
	case Qt::Key_S:
		moveBack = 1;
		break;
	case Qt::Key_A:
		moveLeft = 1;
		break;
	case Qt::Key_D:
		moveRight = 1;
		break;
	case Qt::Key_Q:
		moveDown = 1;
		break;
	case Qt::Key_E:
		moveUp = 1;
		break;
	}
	CRenderWidget::keyPressEvent(event);
}

void CViewportWidget::keyReleaseEvent(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_W:
		moveForward = 0;
		break;
	case Qt::Key_S:
		moveBack = 0;
		break;
	case Qt::Key_A:
		moveLeft = 0;
		break;
	case Qt::Key_D:
		moveRight = 0;
		break;
	case Qt::Key_Q:
		moveDown = 0;
		break;
	case Qt::Key_E:
		moveUp = 0;
		break;
	}
	CRenderWidget::keyReleaseEvent(event);
}

void CViewportWidget::wheelEvent(QWheelEvent* event)
{
	if (camera && camera->bOrthographic)
	{
		orthoZoom = FMath::Clamp(orthoZoom - FMath::Clamp(event->angleDelta().y(), -1, 1), 1, 100);
	}
	else if (bMouseRight || mode == ECameraControlMode::Orbit)
	{
		int d = event->angleDelta().y() > 0 ? 1 : (event->angleDelta().y() < 0 ? -1 : 0);
		cameraSpeed = FMath::Clamp(cameraSpeed + d, 1, 14);
	}
}

void CViewportWidget::showEvent(QShowEvent* event)
{
	CRenderWidget::showEvent(event);

	/*if (camera)
		camera->renderTarget = GetSwapChain()->GetFrameBuffer();*/
}

void CViewportWidget::resizeEvent(QResizeEvent* event)
{
	CRenderWidget::resizeEvent(event);

	/*if (camera)
		camera->renderTarget = GetSwapChain()->GetFrameBuffer();*/
}

void CViewportWidget::DoMousePick(const QPointF& mousePos)
{
	FRay ray = FRay::MouseToRay(camera, { (float)mousePos.x(), (float)mousePos.y() }, { (float)width(), (float)height() });
	ray.direction = ray.direction.Normalize();

	auto* scene = gWorld->GetRenderScene();

	FPrimitiveHitInfo hit;

	if (scene->RayCast(ray.origin, ray.direction, &hit))
	{
		CEntity* ent = nullptr;

		TObjectPtr<CObject> obj = hit.hitProxy->GetOwner();
		if (auto comp = CastChecked<CSceneComponent>(obj); comp)
		{
			ent = comp->GetEntity();
		}

		if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier)
		{
			if (gEditorEngine()->IsObjectSelected(ent))
				gEditorEngine()->RemoveSelectedObject(ent);
			else
				gEditorEngine()->AddSelectedObject(ent);
		}
		else
			gEditorEngine()->SelectObject(ent);
	}
	else
		gEditorEngine()->ClearSelection();
}
