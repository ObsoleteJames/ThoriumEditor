#pragma once

#include "DockWidget.h"
#include "Console.h"

class QLineEdit;
class QTextEdit;
class QCompleter;
class QStringListModel;

class CConsoleWidget : public ads::CDockWidget
{
	Q_OBJECT

public:
	CConsoleWidget(QWidget* parent = nullptr);
	virtual ~CConsoleWidget();

private:
	void OnLog(const FConsoleMsg& msg);
	bool eventFilter(QObject* obj, QEvent* ev) override;

private:
	QTextEdit* consoleLog;
	QLineEdit* input;

	QCompleter* completion;
	QStringListModel* completionModel;

	SizeType onLogBinding;
};
