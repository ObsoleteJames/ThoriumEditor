
#include "RenderWidget.h"
#include "Window.h"
#include "EditorEngine.h"
#include <QResizeEvent>

#include "Rendering/GraphicsInterface.h"

EMouseButton ConvertMouseBtn(Qt::MouseButton btn)
{
	switch (btn)
	{
	case Qt::LeftButton:
		return EMouseButton::LEFT;
	case Qt::RightButton:
		return EMouseButton::RIGHT;
	case Qt::MiddleButton:
		return EMouseButton::MIDDLE;
	case Qt::BackButton:
		return EMouseButton::MOUSE4;
	case Qt::ForwardButton:
		return EMouseButton::MOUSE5;
	case Qt::ExtraButton3:
		return EMouseButton::MOUSE6;
	case Qt::ExtraButton4:
		return EMouseButton::MOUSE7;
	case Qt::ExtraButton5:
		return EMouseButton::MOUSE8;
	}
	return EMouseButton::NONE;
}

EKeyCode ConvertKey(Qt::Key key)
{
	switch (key)
	{
	case Qt::Key_Escape:
		return EKeyCode::ESCAPE;
	case Qt::Key_Tab:
		return EKeyCode::TAB;
	case Qt::Key_Backspace:
		return EKeyCode::BACKSPACE;
	case Qt::Key_Enter:
		return EKeyCode::ENTER;
	case Qt::Key_Insert:
		return EKeyCode::INSERT;
	case Qt::Key_Delete:
		return EKeyCode::KEY_DELETE;
	case Qt::Key_Pause:
		return EKeyCode::PAUSE;
	case Qt::Key_Print:
		return EKeyCode::PRINT_SCREEN;
	case Qt::Key_Home:
		return EKeyCode::HOME;
	case Qt::Key_End:
		return EKeyCode::END;
	case Qt::Key_Left:
		return EKeyCode::LEFT;
	case Qt::Key_Up:
		return EKeyCode::UP;
	case Qt::Key_Down:
		return EKeyCode::DOWN;
	case Qt::Key_Right:
		return EKeyCode::RIGHT;
	case Qt::Key_PageUp:
		return EKeyCode::PAGE_UP;
	case Qt::Key_PageDown:
		return EKeyCode::PAGE_DOWN;
	case Qt::Key_Shift:
		return EKeyCode::LEFT_SHIFT;
	case Qt::Key_Control:
		return EKeyCode::LEFT_CONTROL;
	case Qt::Key_Alt:
		return EKeyCode::LEFT_ALT;
	case Qt::Key_CapsLock:
		return EKeyCode::CAPS_LOCK;
	case Qt::Key_NumLock:
		return EKeyCode::NUM_LOCK;
	case Qt::Key_ScrollLock:
		return EKeyCode::SCROLL_LOCK;
	case Qt::Key_F1:
		return EKeyCode::F1;
	case Qt::Key_F2:
		return EKeyCode::F2;
	case Qt::Key_F3:
		return EKeyCode::F3;
	case Qt::Key_F4:
		return EKeyCode::F4;
	case Qt::Key_F5:
		return EKeyCode::F5;
	case Qt::Key_F6:
		return EKeyCode::F6;
	case Qt::Key_F7:
		return EKeyCode::F7;
	case Qt::Key_F8:
		return EKeyCode::F8;
	case Qt::Key_F9:
		return EKeyCode::F9;
	case Qt::Key_F10:
		return EKeyCode::F10;
	case Qt::Key_F11:
		return EKeyCode::F11;
	case Qt::Key_F12:
		return EKeyCode::F12;
	case Qt::Key_F13:
		return EKeyCode::F13;
	case Qt::Key_F14:
		return EKeyCode::F14;
	case Qt::Key_F15:
		return EKeyCode::F15;
	case Qt::Key_F16:
		return EKeyCode::F16;
	case Qt::Key_F17:
		return EKeyCode::F17;
	case Qt::Key_F18:
		return EKeyCode::F18;
	case Qt::Key_F19:
		return EKeyCode::F19;
	case Qt::Key_F20:
		return EKeyCode::F20;
	case Qt::Key_F21:
		return EKeyCode::F21;
	case Qt::Key_F22:
		return EKeyCode::F22;
	case Qt::Key_F23:
		return EKeyCode::F23;
	case Qt::Key_F24:
		return EKeyCode::F24;
	case Qt::Key_F25:
		return EKeyCode::F25;
	case Qt::Key_Menu:
		return EKeyCode::MENU;
	case Qt::Key_Space:
		return EKeyCode::SPACE;
	case Qt::Key_Equal:
		return EKeyCode::EQUAL;
	case Qt::Key_Minus:
		return EKeyCode::MINUS;
	case Qt::Key_Apostrophe:
		return EKeyCode::APOSTROPHE;
	case Qt::Key_Comma:
		return EKeyCode::COMMA;
	case Qt::Key_Period:
		return EKeyCode::PERIOD;
	case Qt::Key_Slash:
		return EKeyCode::SLASH;
	case Qt::Key_0:
		return EKeyCode::KEY_0;
	case Qt::Key_1:
		return EKeyCode::KEY_1;
	case Qt::Key_2:
		return EKeyCode::KEY_2;
	case Qt::Key_3:
		return EKeyCode::KEY_3;
	case Qt::Key_4:
		return EKeyCode::KEY_4;
	case Qt::Key_5:
		return EKeyCode::KEY_5;
	case Qt::Key_6:
		return EKeyCode::KEY_6;
	case Qt::Key_7:
		return EKeyCode::KEY_7;
	case Qt::Key_8:
		return EKeyCode::KEY_8;
	case Qt::Key_9:
		return EKeyCode::KEY_9;
	case Qt::Key_Semicolon:
		return EKeyCode::SEMICOLON;
	case Qt::Key_A:
		return EKeyCode::A;
	case Qt::Key_B:
		return EKeyCode::B;
	case Qt::Key_C:
		return EKeyCode::C;
	case Qt::Key_D:
		return EKeyCode::D;
	case Qt::Key_E:
		return EKeyCode::E;
	case Qt::Key_F:
		return EKeyCode::F;
	case Qt::Key_G:
		return EKeyCode::G;
	case Qt::Key_H:
		return EKeyCode::H;
	case Qt::Key_I:
		return EKeyCode::I;
	case Qt::Key_J:
		return EKeyCode::J;
	case Qt::Key_K:
		return EKeyCode::K;
	case Qt::Key_L:
		return EKeyCode::L;
	case Qt::Key_M:
		return EKeyCode::M;
	case Qt::Key_N:
		return EKeyCode::N;
	case Qt::Key_O:
		return EKeyCode::O;
	case Qt::Key_P:
		return EKeyCode::P;
	case Qt::Key_Q:
		return EKeyCode::Q;
	case Qt::Key_R:
		return EKeyCode::R;
	case Qt::Key_S:
		return EKeyCode::S;
	case Qt::Key_T:
		return EKeyCode::T;
	case Qt::Key_U:
		return EKeyCode::U;
	case Qt::Key_V:
		return EKeyCode::V;
	case Qt::Key_W:
		return EKeyCode::W;
	case Qt::Key_X:
		return EKeyCode::X;
	case Qt::Key_Y:
		return EKeyCode::Y;
	case Qt::Key_Z:
		return EKeyCode::Z;
	case Qt::Key_Backslash:
		return EKeyCode::BACKSLASH;
	case Qt::Key_BracketLeft:
		return EKeyCode::LEFT_BRACKET;
	case Qt::Key_BracketRight:
		return EKeyCode::RIGHT_BRACKET;
	}

	return EKeyCode::UNKOWN;
}

EInputMod ConvertMod(Qt::KeyboardModifiers mod)
{
	int m = IM_NONE;
	if (mod & Qt::ShiftModifier)
		m |= IM_SHIFT;
	if (mod & Qt::ControlModifier)
		m |= IM_CONTROL;
	if (mod & Qt::AltModifier)
		m |= IM_ALT;
	if (mod & Qt::KeypadModifier)
		m |= IM_NUM_LOCK;

	return (EInputMod)m;
}

class EDITOR_API CRenderWindow : public IBaseWindow
{
public:
	CRenderWindow(CRenderWidget* w) : widget(w)
	{
		width = 640;
		height = 480;
		bIsGlfwWindow = true;
	}

	void* GetNativeHandle() override
	{
		return (void*)widget->winId();
	}

	void OnResize(int w, int h)
	{
		width = w;
		height = h;
		OnWindowResize.Invoke(w, h);
	}

	void OnMouseMove(int x, int y)
	{
		mouseX = x;
		mouseY = y;
		OnCursorMove.Invoke((double)x, (double)y);
	}

	CRenderWidget* widget;
};

CRenderWidget::CRenderWidget(QWidget* parent /*= nullptr*/) : QWidget(parent)
{
	//setFocusPolicy(Qt::StrongFocus);
	setAutoFillBackground(false);
	setMouseTracking(true);
	setMinimumSize(64, 64);

	setAttribute(Qt::WA_NativeWindow);
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_NoSystemBackground);

	windowInterface = new CRenderWindow(this);

	//swapChain = gGHI->CreateSwapChain(windowInterface);
}

CRenderWidget::~CRenderWidget()
{
	delete windowInterface;
}

QPaintEngine* CRenderWidget::paintEngine() const
{
	return nullptr;
}

void CRenderWidget::paintEvent(QPaintEvent* event)
{
}

void CRenderWidget::showEvent(QShowEvent* event)
{
	if (!swapChain)
		swapChain = gGHI->CreateSwapChain(windowInterface);

	QWidget::showEvent(event);
}

void CRenderWidget::resizeEvent(QResizeEvent* event)
{
	int width, height;
	width = event->size().width();
	height = event->size().height();

	class RenderResizeEvent : public IEditorEvent
	{
	public:
		RenderResizeEvent(CRenderWidget* w, IBaseWindow* wnd, int width, int height) : widget(w), window(wnd), w(width), h(height) {}

		void Exec() override
		{
			widget->GetSwapChain()->Resize(w, h);
			window->OnWindowResize.Invoke(w, h);
		}

		int w, h;
		CRenderWidget* widget;
		IBaseWindow* window;
	};
	if (swapChain)
		gEditorEngine()->PushEvent(new RenderResizeEvent(this, windowInterface, width, height));
}

class FMouseBtnEvent : public IEditorEvent
{
public:
	FMouseBtnEvent(IBaseWindow* w, EMouseButton b, EInputAction a, EInputMod m) : wnd(w), btn(b), action(a), mod(m)
	{
	}

	void Exec() override
	{
		wnd->OnMouseButton.Invoke(btn, action, mod);
	}

	IBaseWindow* wnd;
	EMouseButton btn;
	EInputAction action;
	EInputMod mod;
};

void CRenderWidget::mousePressEvent(QMouseEvent* event)
{
	FMouseBtnEvent* e = new FMouseBtnEvent(windowInterface, ConvertMouseBtn(event->button()), IE_PRESS, ConvertMod(event->modifiers()));
	gEditorEngine()->PushEvent(e);

	QWidget::mousePressEvent(event);
}

void CRenderWidget::mouseReleaseEvent(QMouseEvent* event)
{
	FMouseBtnEvent* e = new FMouseBtnEvent(windowInterface, ConvertMouseBtn(event->button()), IE_PRESS, ConvertMod(event->modifiers()));
	gEditorEngine()->PushEvent(e);

	QWidget::mouseReleaseEvent(event);
}

class FMouseMoveEvent : public IEditorEvent
{
public:
	FMouseMoveEvent(IBaseWindow* wnd, int _x, int _y) : window(wnd), x(_x), y(_y) {}

	void Exec() override
	{
		window->OnCursorMove.Invoke(x, y);
	}

	int x, y;
	IBaseWindow* window;
};

void CRenderWidget::mouseMoveEvent(QMouseEvent* event)
{
	auto p = event->localPos();
	FMouseMoveEvent* e = new FMouseMoveEvent(windowInterface, p.x(), p.y());
	gEditorEngine()->PushEvent(e);

	QWidget::mouseMoveEvent(event);
}

class FKeyEvent : public IEditorEvent
{
public:
	FKeyEvent(IBaseWindow* w, EKeyCode b, EInputAction a, EInputMod m) : wnd(w), key(b), action(a), mod(m)
	{
	}

	void Exec() override
	{
		wnd->OnKeyEvent.Invoke(key, action, mod);
	}

	IBaseWindow* wnd;
	EKeyCode key;
	EInputAction action;
	EInputMod mod;
};

void CRenderWidget::keyPressEvent(QKeyEvent* event)
{
	FKeyEvent* e = new FKeyEvent(windowInterface, ConvertKey((Qt::Key)event->key()), IE_PRESS, ConvertMod(event->modifiers()));
	gEditorEngine()->PushEvent(e);

	QWidget::keyPressEvent(event);
}

void CRenderWidget::keyReleaseEvent(QKeyEvent* event)
{
	FKeyEvent* e = new FKeyEvent(windowInterface, ConvertKey((Qt::Key)event->key()), IE_PRESS, ConvertMod(event->modifiers()));
	gEditorEngine()->PushEvent(e);

	QWidget::keyPressEvent(event);
}
