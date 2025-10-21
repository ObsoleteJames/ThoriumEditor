#pragma once

#include "Windows/ToolsWindow.h"

class QLabel;
class QTreeWidget;
class QSplitter;
class QWidget;
class QFrame;
class QScrollArea;
class QVBoxLayout;
class QTreeWidgetItem;

class CObjectDebugger : public CToolsWindow
{
	Q_OBJECT
	ToolsWindowBody(CObjectDebugger, "Objects List", false)

public:
	CObjectDebugger() = default;

	bool Shutdown() override { return true; }
	void SetupUi() override;

	void GenerateTree();
	
	QTreeWidgetItem* GetClassItem(const FString& type);

private:
	QTreeWidget* treeView;

	TArray<QTreeWidgetItem*> classItems;

	QLabel* lblName;
	QLabel* lblUsers;
	QLabel* lblDelete;

};
