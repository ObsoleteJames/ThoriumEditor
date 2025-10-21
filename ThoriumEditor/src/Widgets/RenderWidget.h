#pragma once

#include "Editor.h"
#include <QWidget>

class IBaseWindow;
class CRenderWindow;
class ISwapChain;

class EDITOR_API CRenderWidget : public QWidget
{
	Q_OBJECT

public:
	CRenderWidget(QWidget* parent = nullptr);
	~CRenderWidget();

	inline IBaseWindow* GetWindow() const { return (IBaseWindow*)windowInterface; }
	inline ISwapChain* GetSwapChain() const { return swapChain; }

protected:
	QPaintEngine* paintEngine() const override;
	void paintEvent(QPaintEvent* event) override;

	void showEvent(QShowEvent* event) override;

	void resizeEvent(QResizeEvent* event) override;

	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;

private:
	CRenderWindow* windowInterface = nullptr;
	ISwapChain* swapChain = nullptr;

};
