
#include "LogWnd.h"

#include <QLabel>
#include <QBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QCompleter>
#include <QStringListModel>
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QSplitter>

CLogWindow::CLogWindow(QWidget* parent) : ads::CDockWidget("Log", parent)
{
	QWidget* rootWidget = new QWidget(this);
	QVBoxLayout* layout = new QVBoxLayout();
	rootWidget->setLayout(layout);

	setIcon(QIcon(":/icons/wnd_console.svg"));
	setWidget(rootWidget);

	QSplitter* splitter = new QSplitter(Qt::Vertical, this);
	
	
}
