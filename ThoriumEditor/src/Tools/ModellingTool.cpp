
#include "ModellingTool.h"
#include "EditorWindow.h"

#include <QBoxLayout>
#include <QLabel>

CModellingTool::CModellingTool()
{
	setObjectName("Modelling Tool");
}

void CModellingTool::Init()
{
	toolWindow = new ads::CDockWidget("Modelling Tool", gEditorWindow);
	toolWindow->setFeature(ads::CDockWidget::DockWidgetClosable, false);

	QWidget* widget = new QWidget(toolWindow);
	QVBoxLayout* layout = new QVBoxLayout(toolWindow);
	widget->setLayout(layout);

	toolWindow->setWidget(widget);

	QLabel* txt = new QLabel("Hello!\n Bitch!!!", toolWindow);
	layout->addWidget(txt);

	gEditorWindow->sceneDockManager->addDockWidget(ads::LeftDockWidgetArea, toolWindow);
	toolWindow->toggleView(false);
}

void CModellingTool::Enable()
{
	toolWindow->toggleView(true);
}

void CModellingTool::Disable()
{
	toolWindow->toggleView(false);
}
