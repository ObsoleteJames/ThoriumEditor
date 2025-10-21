
#include "ObjectTool.h"
#include "EditorWindow.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>

CObjectTool::CObjectTool()
{
	setObjectName("Object Tool");
}

void CObjectTool::Init()
{
	toolWindow = new ads::CDockWidget("Object Tool", gEditorWindow);
	toolWindow->setIcon(QIcon(":/icons/entity.svg"));
	icon = toolWindow->icon();
	//toolWindow->setFeature(ads::CDockWidget::DockWidgetClosable, false);

	QWidget* widget = new QWidget(toolWindow);
	QVBoxLayout* layout = new QVBoxLayout(toolWindow);
	widget->setLayout(layout);

	toolWindow->setWidget(widget);

	QGroupBox* group = new QGroupBox("Create Objects", widget);
	QGridLayout* layout2 = new QGridLayout(toolWindow);
	group->setLayout(layout2);

	layout->addWidget(group);

	QPushButton* btn = new QPushButton("Entity", toolWindow);
	layout2->addWidget(btn, 0, 0);

	btn = new QPushButton("Pawn", toolWindow);
	layout2->addWidget(btn, 0, 1);

	btn = new QPushButton("Point Light", toolWindow);
	layout2->addWidget(btn, 1, 0);

	btn = new QPushButton("Sun Light", toolWindow);
	layout2->addWidget(btn, 1, 1);

	layout->addStretch(1);

	gEditorWindow->getDockManager()->addDockWidget(ads::LeftDockWidgetArea, toolWindow);
	toolWindow->toggleView(false);
}

void CObjectTool::Enable()
{
	toolWindow->toggleView(true);
}

void CObjectTool::Disable()
{
	toolWindow->toggleView(false);
}
