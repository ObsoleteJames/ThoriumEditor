
#include "ObjectTool.h"
#include "EditorWindow.h"
#include "EditorEngine.h"
#include "Gizmo.h"
#include "Game/Events.h"

#include "Game/Entity.h"
#include "Game/Components/SceneComponent.h"

#include "Widgets/ViewportWidget.h"

#include "UndoActions/EntityTranslateAction.h"
#include <QUndoStack>
#include <QBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>

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

	connect(gEditorWindow, &CEditorWindow::onGizmoModeChanged, this, [=](EGizmoMode m) { gizmo->mode = m; });
}

void CObjectTool::Enable()
{
	bEnabled = true;
	toolWindow->toggleView(true);
}

void CObjectTool::Disable()
{
	bEnabled = false;
	toolWindow->toggleView(false);
}

void CObjectTool::Update()
{
}

void CObjectTool::GameUpdate()
{
	if (!bEnabled || !gWorld || !gWorld->GetRenderScene())
		return;

	gizmo->renderScene = gWorld->GetRenderScene();

	FTransform gizmoTransform = FTransform();
	FBounds gizmoBounds{};
	bool bEnbleGizmo = false;
	if (gEditorEngine->selectedObjects.Size() > 1)
	{
		for (auto& obj : gEditorEngine->selectedObjects)
		{
			CEntity* ent = Cast<CEntity>(obj);
			if (ent)
			{
				if (gizmo->pivotMode == Pivot_BoundingBox)
				{
					if (gizmoBounds.extents == FVector::zero)
						gizmoBounds = ent->GetBounds();
					else
						gizmoBounds = gizmoBounds.Combine(ent->GetBounds());
				}
				else if (gizmo->pivotMode == Pivot_Center)
					gizmoTransform.position += ent->RootComponent()->GetWorldPosition();
			}
		}

		if (gizmo->pivotMode == Pivot_Active)
		{
			CEntity* active = Cast<CEntity>(gEditorEngine->activeObject);
			if (active)
				gizmoTransform = active->RootComponent()->GetWorldTransform();
		}
		else if (gizmo->pivotMode == Pivot_BoundingBox)
			gizmoTransform.position = gizmoBounds.position;
		else
			gizmoTransform.position /= gEditorEngine->selectedObjects.Size();

		bEnbleGizmo = true;
	}
	else if (gEditorEngine->activeObject)
	{
		CEntity* ent = Cast<CEntity>(gEditorEngine->activeObject);
		if (ent)
		{
			gizmoTransform = ent->RootComponent()->GetWorldTransform();

			bEnbleGizmo = true;
		}
	}

	if (bEnbleGizmo)
	{
		FRay ray;
		if (gEditorWindow->activeViewport)
		{
			QSize wndSize = gEditorWindow->activeViewport->size();
			ray = FRay::MouseToRay(gEditorWindow->activeViewport->GetCamera(), FVector2(mousePos.x(), mousePos.y()), FVector2(wndSize.width(), wndSize.height()));
		}

		if (gizmo->mode == Gizmo_Rotate && gEditorEngine->bAngleSnap ^ bCtrl)
			gizmo->SetSnap(FVector(gEditorEngine->angleSnap).Radians());
		else if (gizmo->mode == Gizmo_Translate && gEditorEngine->bGridSnap ^ bCtrl)
			gizmo->SetSnap(FVector(gEditorEngine->gridSize));
		else
			gizmo->ClearSnap();

		static FTransform prevDelta;
		FTransform delta;
		static bool wasManipulating = false;
		bool b = gizmo->Manipulate(gizmoTransform.position, gizmoTransform.rotation, gizmoTransform.scale, ray, mouseBtns, &delta);
		if (b && !wasManipulating)
			prevDelta = delta;

		if (b)
		{
			FTransform d = prevDelta.Inverse() + delta;
			prevDelta = delta;
			for (auto& obj : gEditorEngine->selectedObjects)
			{
				CEntity* ent = Cast<CEntity>(obj);
				if (ent)
				{
					auto* root = ent->RootComponent();
					FTransform t = root->GetWorldTransform() + d;
					root->SetPosition(t.position);
					root->SetRotation(t.rotation);
					root->SetScale(t.scale);
				}
			}
		}
		else if (wasManipulating) // done manipulating
		{
			auto* cmd = new CmdTranslateEntity(gEditorEngine->GetSelectedObjects<CEntity>(), prevDelta);
			gEditorWindow->sceneUndoStack->push(cmd); 
		}

		wasManipulating = b;
	}
}

bool CObjectTool::viewportEvent(QObject* obj, QEvent* ev)
{
	if (ev->type() == QEvent::MouseMove)
	{
		QMouseEvent* mouseEv = static_cast<QMouseEvent*>(ev);
		mousePos = mouseEv->localPos();

		if (gizmo->hoverAxis != 0 && gizmo->bDragging)
			return true;
	}
	if (ev->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent* mouseEv = static_cast<QMouseEvent*>(ev);
		mouseBtns |= mouseEv->button();

		if (gizmo->hoverAxis != 0 && mouseBtns & Qt::LeftButton)
			return true;
	}
	if (ev->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent* mouseEv = static_cast<QMouseEvent*>(ev);
		mouseBtns &= ~mouseEv->button();

		if (gizmo->hoverAxis != 0 && mouseBtns & Qt::LeftButton)
			return true;
	}
	if (ev->type() == QEvent::KeyPress)
	{
		QKeyEvent* keyEv = (QKeyEvent*)ev;
		if (keyEv->key() == Qt::Key_Control)
		{
			bCtrl = true;
			return true;
		}
	}
	if (ev->type() == QEvent::KeyRelease)
	{
		QKeyEvent* keyEv = (QKeyEvent*)ev;
		if (keyEv->key() == Qt::Key_Control)
		{
			bCtrl = false;
			return true;
		}
	}

	return false;
}
