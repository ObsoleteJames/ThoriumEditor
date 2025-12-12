#pragma once

#include <QWidget>

class QListView;
class QTableView;
class QStandardItemModel;

/* 
 *	Asset Browser Widget
 *	Displays the assets available in the project.
 */
class CAssetBrowserWidget : public QWidget
{
	Q_OBJECT

public:
	CAssetBrowserWidget(QWidget* parent = nullptr);
	virtual ~CAssetBrowserWidget();

	QListView* listView;
	QTableView* tableView;
	QStandardItemModel* assetModel;
};
