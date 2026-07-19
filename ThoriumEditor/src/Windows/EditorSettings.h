#pragma once

#include "Windows/ToolsWindow.h"

class QSplitter;
class QListWidget;
class QStackedWidget;
class CSettingsPage;

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

	void AddPage(QWidget* page, const QString& title);
	CSettingsPage* GetPage(const QString& title, bool bCreateNew = true);

private slots:
	void SwitchPage(int index);

private:
	QListWidget* settingsIndex;
	QStackedWidget* settingsView;
	QSplitter* splitter;

	CSettingsPage* general;
	CSettingsPage* appearance;

	TArray<CSettingsPage*> pages;

	int curPage = 0;
};
