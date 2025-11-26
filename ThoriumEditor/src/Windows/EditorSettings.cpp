
#include "EditorSettings.h"

#include <QTreeWidget>
#include <QSplitter>
#include <QBoxLayout>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QSpinBox>
#include <QHeaderView>

SDK_REGISTER_WINDOW(CEditorSettingsWnd, "Editor Settings", "Edit", NULL);

class CGridViewDelegate : public QStyledItemDelegate
{
public:
	CGridViewDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		QSize sz = QStyledItemDelegate::sizeHint(option, index);
		sz.setHeight(sz.height() + 6);
		return sz;
	}
};

void CEditorSettingsWnd::SetupUi()
{
	auto r = geometry();
	r.setSize({ 840, 520 });
	setGeometry(r);

	QWidget* widget = new QWidget(this);
	setCentralWidget(widget);

	QHBoxLayout* layout = new QHBoxLayout(widget);
	widget->setLayout(layout);

	splitter = new QSplitter(Qt::Horizontal, this);
	layout->addWidget(splitter);

	settingsIndex = new QTreeWidget(this);
	settingsIndex->setHeaderHidden(true);
	splitter->addWidget(settingsIndex);

	(new QTreeWidgetItem(settingsIndex))->setText(0, "General");
	(new QTreeWidgetItem(settingsIndex))->setText(0, "Appearance");
	(new QTreeWidgetItem(settingsIndex))->setText(0, "User");

	settingsView = new QWidget(this);
	settingsView->setLayout(new QHBoxLayout());
	settingsView->layout()->setContentsMargins(0, 0, 0, 0);
	splitter->addWidget(settingsView);

	splitter->setStretchFactor(0, 2);
	splitter->setStretchFactor(1, 6);

	sGeneral = new QTreeWidget(this);
	sGeneral->setHeaderLabels({ "Name", "Value" });
	sGeneral->setItemDelegate(new CGridViewDelegate(this));
	sGeneral->header()->resizeSection(0, 250);
	settingsView->layout()->addWidget(sGeneral);

	auto* item = new QTreeWidgetItem(sGeneral);
	item->setText(0, "Uhh");
	item->setExpanded(true);

	auto* item2 = new QTreeWidgetItem();
	item2->setText(0, "AAA");
	item->addChild(item2);

	auto* intItem = new QSpinBox(this);
	intItem->setValue(6969);
	sGeneral->setItemWidget(item2, 1, intItem);
}
