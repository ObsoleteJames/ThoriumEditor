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

private:
	QTreeWidget* settingsIndex;
	QWidget* settingsView;
	QSplitter* splitter;

	QTreeWidget* sGeneral;
	QTreeWidget* sAppearance;

};
