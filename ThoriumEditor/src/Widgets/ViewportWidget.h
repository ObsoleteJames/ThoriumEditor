#pragma once

#include "RenderWidget.h"
#include "Math/Vectors.h"
#include <QPoint>

class QComboBox;

enum class ECameraControlMode
{
	Disabled,
	FreeMode,
	Orbit,
	Ortho
};

enum ECameraView
{
	Cam3DPerspective,
	Cam3DOrtho,
	Cam2DTop,
	Cam2DFront,
	Cam2DSide,
};

class CViewportWidget : public CRenderWidget
{
	Q_OBJECT

public:
	CViewportWidget(QWidget* parent = nullptr, bool bCreateCam = false);
	virtual ~CViewportWidget();

public:
	void SetCamera(CCameraProxy* cam);
	inline CCameraProxy* GetCamera() const { return camera; }

	inline void SetControlMode(ECameraControlMode m) { mode = m; }

	inline FVector GetMoveVector() const { return FVector(moveLeft + -moveRight, -moveForward + moveBack, 0); }

	void SetViewMode(ECameraView view);
	inline ECameraView GetViewMode() const { return camView; }

protected:
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;

	void wheelEvent(QWheelEvent* event) override;

	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

	bool event(QEvent* event) override;

	//void DoMousePick(const QPointF& mousePos);

Q_SIGNALS:
	void onMousePick(const FRay& ray, bool bIsRightMouse);

public Q_SLOTS:
	void OnUpdate();

public:
	float camFov = 90.f;

private:
	CCameraProxy* camera = nullptr;
	bool bOwnsCamera = false;

	QComboBox* comboViewMode;

	ECameraControlMode mode = ECameraControlMode::FreeMode;
	ECameraView camView = Cam3DPerspective;

	QPoint mouseClickPos;
	int cameraSpeed = 4;
	float cameraPitch = 0;
	float cameraYaw = 0;
	FVector orbitPos;

	float curSpeed = 0.f;
	int orthoZoom = 7;

	int prevMouseX = 0;
	int prevMouseY = 0;

	int8 moveForward : 1;
	int8 moveBack : 1;
	int8 moveLeft : 1;
	int8 moveRight : 1;
	int8 moveUp : 1;
	int8 moveDown : 1;

	float mouseDeltaX;
	float mouseDeltaY;

	bool bMouseLeft : 1;
	bool bMouseRight : 1;
	bool bMouseMiddle : 1;

	bool bRotateCam = false;

};
