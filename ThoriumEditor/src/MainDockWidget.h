#pragma once

#include <DockWidget.h>
#include <QMainWindow>
#include <QToolBar>
#include <QMenuBar>
#include <QMenu>

/**
 * A CDockWidget that can host QToolBars and QMenuBars with native auto-docking behavior
 * Uses a QMainWindow internally to provide toolbar docking to edges
 */
class CMainDockWidget : public ads::CDockWidget
{
	Q_OBJECT

public:
	explicit CMainDockWidget(const QString& title, QWidget* parent = nullptr);
	~CMainDockWidget();

	/**
	 * Add a toolbar to this dock widget
	 * Toolbars can be docked to Top, Bottom, Left, or Right edges
	 */
	QToolBar* addToolBar(const QString& title);

	/**
	 * Get or create the menu bar for this dock widget
	 */
	QMenuBar* menuBar() const;

	/**
	 * Add a menu to the menu bar
	 */
	QMenu* addMenu(const QString& title);

	/**
	 * Get the main widget area where content should be placed
	 */
	QWidget* getContentWidget() const;

	/**
	 * Set the central widget (content area)
	 */
	void setContentWidget(QWidget* widget);

	inline QMainWindow* MainWindow() const { return mainWindow; }

private:
	QMainWindow* mainWindow;
	QWidget* contentWidget;
};
