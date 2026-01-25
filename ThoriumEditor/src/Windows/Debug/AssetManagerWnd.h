#pragma once

#include "Windows/ToolsWindow.h"

class QTreeWidget;
class QSplitter;
class QWidget;
class QFrame;
class QScrollArea;
class QVBoxLayout;

class CAssetManagerWnd : public CToolsWindow
{
	Q_OBJECT
	ToolsWindowBody(CAssetManagerWnd, "Asset Manager", false)

public:
	CAssetManagerWnd() = default;

protected:
	virtual bool Shutdown() override;
	virtual void SetupUi() override;

	void UserSaveState(QSettings& settings) override;
	void UserRestoreState(QSettings& settings) override;

private:
	QTreeWidget* treeView;
	QWidget* Content;
	QScrollArea* PropertiesScroll;
	QFrame* Properties;
	QVBoxLayout* propertiesLayout;

	QSplitter* splitter;

};