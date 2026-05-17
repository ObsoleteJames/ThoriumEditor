#pragma once

#include "DockWidget.h"
#include <Util/Map.h>
#include "Object/Object.h"
#include "Widgets/TreeDataItem.h"

class QLineEdit;
class QTreeWidget;
class CEntity;
class CWorld;

struct FOutlinerFolder
{
	FString name;
	QTreeWidgetItem* item = nullptr;

	TArray<FOutlinerFolder> children;
	TArray<SizeType> entities; // ent IDs
};

class COutlinerWindow : public ads::CDockWidget
{
	Q_OBJECT

public:
	COutlinerWindow(QWidget* parent = nullptr);
	virtual ~COutlinerWindow();

	void Clear();

private slots:
	void Update();

	void selectionChanged();

	void LoadSceneTree();
	void SaveSceneTree();

Q_SIGNALS:
	void entitySelected(CEntity*);

public:
	QTreeWidgetItem* sceneItem;

	//	 entId  -  folderItem
	TMap<SizeType, QTreeWidgetItem*> entityFolderLut;

private:
	TMap<SizeType, QTreeWidgetItem*> entityItems;

	CWorld* curWorld = nullptr;

	QLineEdit* filter;
	QTreeWidget* outlinerTree;

	QTimer* updateTimer;
};
