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
	TArray<SizeType> entities;
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

Q_SIGNALS:
	void entitySelected(CEntity*);

private:
	TMap<SizeType, QTreeWidgetItem*> entityItems;

	QTreeWidgetItem* sceneItem;

	//	 entId  -  folderItem
	TMap<SizeType, QTreeWidgetItem*> entityFolderLut;

	FOutlinerFolder folderRoot;

	QLineEdit* filter;
	QTreeWidget* outlinerTree;
};
