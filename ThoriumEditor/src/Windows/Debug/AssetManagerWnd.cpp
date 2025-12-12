
#include "AssetManagerWnd.h"
#include "Assets/Asset.h"

#include <QToolBox>
#include <QSettings>
#include <QBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QTreeWidget>
#include <QSplitter>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QSpacerItem>

SDK_REGISTER_WINDOW(CAssetManagerWnd, "Asset Manager", "Debug", NULL);

bool CAssetManagerWnd::Shutdown()
{
	SaveState();
    return true;
}

void CAssetManagerWnd::SetupUi()
{
	CToolsWindow::SetupUi();

	setGeometry(QRect(0, 0, 640, 430));
	setWindowTitle("Asset Manager");

	Content = new QWidget(this);
	auto layout = new QVBoxLayout();
	Content->setLayout(layout);

	Properties = new QFrame(Content);
	propertiesLayout = new QVBoxLayout(Properties);
	Properties->setLayout(propertiesLayout);
	Properties->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
	propertiesLayout->setSpacing(0);

	PropertiesScroll = new QScrollArea(Content);
	PropertiesScroll->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	PropertiesScroll->setWidgetResizable(true);
	PropertiesScroll->setWidget(Properties);
	PropertiesScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	treeView = new QTreeWidget(Content);
	treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
	treeView->setColumnCount(2);
	treeView->setColumnWidth(0, 200);
	treeView->setHeaderLabels({ "Name", "Type" });
}

void CAssetManagerWnd::UserSaveState(QSettings& settings)
{
}

void CAssetManagerWnd::UserRestoreState(QSettings& settings)
{
}
