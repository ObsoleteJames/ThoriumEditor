#pragma once

#include "Windows/ToolsWindow.h"

class QSplitter;
class QTreeWidget;

class EDITOR_API CEditorSettingsWnd : public CToolsWindow
{
	Q_OBJECT
	Q_DISABLE_COPY(CEditorSettingsWnd)

	ToolsWindowBody(CEditorSettingsWnd, "Editor Settings", false)

public:
	CEditorSettingsWnd() = default;

	void SetupUi() override;

protected:
	void UserSaveState(QSettings& out) override;
	void UserRestoreState(QSettings& in) override;

private slots:
	void SwitchPage(int index);

private:
	QTreeWidget* settingsIndex;
	QWidget* settingsView;
	QSplitter* splitter;

	QWidget* general;
	QWidget* appearance;

	int curPage = 0;
};
