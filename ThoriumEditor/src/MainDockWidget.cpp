
#include "MainDockWidget.h"
#include <QVBoxLayout>
#include <QMainWindow>

CMainDockWidget::CMainDockWidget(const QString& title, QWidget* parent)
	: ads::CDockWidget(title, parent)
{
	// Create a QMainWindow to leverage its native toolbar docking
	mainWindow = new QMainWindow();
	mainWindow->setWindowFlags(Qt::Widget);
	
	// Enable toolbar docking - allows toolbars to dock to all edges
	mainWindow->setDockOptions(
		QMainWindow::AllowNestedDocks |
		QMainWindow::AllowTabbedDocks |
		QMainWindow::AnimatedDocks
	);

	// Set the QMainWindow as the dock widget's content
	setWidget(mainWindow);
}

CMainDockWidget::~CMainDockWidget()
{
	mainWindow->deleteLater();
}

QToolBar* CMainDockWidget::addToolBar(const QString& title)
{
	return mainWindow->addToolBar(title);
}

QMenuBar* CMainDockWidget::menuBar() const
{
	return mainWindow->menuBar();
}

QMenu* CMainDockWidget::addMenu(const QString& title)
{
	return mainWindow->menuBar()->addMenu(title);
}

QWidget* CMainDockWidget::getContentWidget() const
{
	return contentWidget;
}

void CMainDockWidget::setContentWidget(QWidget* widget)
{
	contentWidget = widget;
	if (contentWidget)
		mainWindow->setCentralWidget(contentWidget);
}
