
#include "ObjectTool.h"
#include "EditorWindow.h"
#include "EditorEngine.h"
#include "Gizmo.h"
#include "Game/Events.h"

#include "Game/Entity.h"
#include "Game/Components/SceneComponent.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>

//class PObjectTool : public CObject
//{
//public:
//	PObjectTool(CObjectTool* tool)
//	{
//		Events::OnUpdate.Bind(this, &PObjectTool::Update);
//	}
//
//	void Update()
//	{
//		if (!gWorld)
//			return;
//
//		tool->gizmo->renderScene = gWorld->GetRenderScene();
//		if (gEditorEngine()->activeObject)
//		{
//			CEntity* ent = Cast<CEntity>(gEditorEngine()->activeObject);
//
//			if (ent)
//			{
//				FVector pos = ent->RootComponent()->GetWorldPosition();
//				FQuaternion rot = ent->RootComponent()->GetWorldRotation();
//				FVector scale = ent->RootComponent()->GetWorldScale();
//				tool->gizmo->Manipulate(nullptr, nullptr, true, pos, rot, scale);
//			}
//		}
//	}
//
//public:
//	CObjectTool* tool;
//};

CObjectTool::CObjectTool()
{
	setObjectName("Object Tool");
	//p = new PObjectTool(this);

	gizmo = new FGizmo();

	Events::OnUpdate.Bind(this, &CObjectTool::GameUpdate);
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

void CObjectTool::Update()
{
}

void CObjectTool::GameUpdate()
{
	if (!gWorld || !gWorld->GetRenderScene())
		return;

	gizmo->renderScene = gWorld->GetRenderScene();
	if (gEditorEngine->activeObject)
	{
		CEntity* ent = Cast<CEntity>(gEditorEngine->activeObject);

		if (ent)
		{
			FVector pos = ent->RootComponent()->GetWorldPosition();
			FQuaternion rot = ent->RootComponent()->GetWorldRotation();
			FVector scale = ent->RootComponent()->GetWorldScale();
			gizmo->Manipulate(nullptr, nullptr, true, pos, rot, scale);
		}
	}
}
