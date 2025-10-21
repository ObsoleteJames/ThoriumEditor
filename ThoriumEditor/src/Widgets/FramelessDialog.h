#pragma once

#include <QDialog>
#include <QLabel>

class QFrame;
class QVBoxLayout;
class QHBoxLayout;

class CFramelessDialog : public QDialog
{
	Q_OBJECT

public:
	CFramelessDialog(QWidget* parent = nullptr);
	~CFramelessDialog();

	void setCentralWidget(QWidget* w);

	void setTitle(const QString& title) { titleLable->setText(title); }

protected:
	void mouseMoveEvent(QMouseEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);
	
private:
	QWidget* centralWidget = nullptr;
	QWidget* titlebarWidget;
	QLabel* titleLable;
	QPushButton* closeButton;
	QPoint lastMousePos;
	int bMove = 0;

};
