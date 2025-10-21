
#include "ObjectDebugger.h"
#include "Object/Object.h"
#include "Object/ObjectManager.h"

#include <QToolBox>
#include <QSettings>
#include <QBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QTreeWidget>
#include <QPushButton>
#include <QSpacerItem>

SDK_REGISTER_WINDOW(CObjectDebugger, "Objects List", "Debug", NULL);

void CObjectDebugger::SetupUi()
{
	CToolsWindow::SetupUi();

	setWindowTitle("Objects List");

	QWidget* content = new QWidget(this);
	auto layout = new QVBoxLayout(content);

	QHBoxLayout* l2 = new QHBoxLayout();
	l2->addSpacing(0);

	QPushButton* btnRefresh = new QPushButton("Refresh", this);
	connect(btnRefresh, &QPushButton::clicked, this, &CObjectDebugger::GenerateTree);
	l2->addWidget(btnRefresh);

	treeView = new QTreeWidget(content);
	treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
	treeView->setColumnCount(2);
	treeView->setColumnWidth(0, 200);
	treeView->setHeaderLabels({ "Name", "Value" });

	layout->addLayout(l2);
	layout->addWidget(treeView);
	setCentralWidget(content);

	GenerateTree();
}

void CObjectDebugger::GenerateTree()
{
	treeView->clear();
	classItems.Clear();

	auto& objs = CObjectManager::GetAllObjects();

	for (auto& it : objs)
	{
		CObject* obj = it.second;
		if (!obj)
			continue;

		FString name = obj->Name();
		SizeType users = obj->GetUserCount();
		bool bDeleted = obj->IsMarkedForDeletion();
		bool bStrong = obj->IsIndestructible();

		FString className = obj->GetClass()->GetInternalName();

		QTreeWidgetItem* classItem = GetClassItem(className);

		QTreeWidgetItem* item = new QTreeWidgetItem(classItem, { name.c_str(), className.c_str() });

		new QTreeWidgetItem(item, { "Users", QString::number(users)});
		new QTreeWidgetItem(item, { "Marked for deletion", bDeleted ? "true" : "false" });
		new QTreeWidgetItem(item, { "Indestructible", bStrong ? "true" : "false" });

	}
}

QTreeWidgetItem* CObjectDebugger::GetClassItem(const FString& type)
{
	QString qType = type.c_str();
	for (auto* i : classItems)
		if (i->text(0) == qType)
			return i;

	QTreeWidgetItem* i = new QTreeWidgetItem(treeView, { qType, QString() });
	classItems.Add(i);
	return i;
}
