#pragma once

#include "Windows/ToolsWindow.h"

class QTreeWidget;
class QSplitter;
class QWidget;
class QFrame;
class QScrollArea;
class QVBoxLayout;
class CTreeModuleItem;

class CModuleDebugger : public CToolsWindow
{
	Q_OBJECT
	ToolsWindowBody(CModuleDebugger, "Module Viewer", false)

public:
	CModuleDebugger() = default;

protected:
	virtual bool Shutdown() override;
	virtual void SetupUi() override;

	void UpdatePropertyList(CTreeModuleItem* item);
	void GenerateTree();

	void UserSaveState(QSettings& settings) override;
	void UserRestoreState(QSettings& settings) override;

private:
	QTreeWidget* treeView;
	QWidget* Content;
	QScrollArea* PropertiesScroll;
	QFrame* Properties;
	QVBoxLayout* propertiesLayout;

	TArray<QWidget*> curProperties;

};
