#pragma once

#include "DockWidget.h"
#include "Console.h"

class QLineEdit;
class QTextEdit;

class CConsoleWidget : public ads::CDockWidget
{
	Q_OBJECT

public:
	CConsoleWidget(QWidget* parent = nullptr);
	virtual ~CConsoleWidget();

private:
	void OnLog(const FConsoleMsg& msg);

private:
	QTextEdit* consoleLog;
	QLineEdit* input;

	SizeType onLogBinding;
};
