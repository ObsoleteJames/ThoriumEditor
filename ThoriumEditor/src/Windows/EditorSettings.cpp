
#include "EditorSettings.h"

#include <QTreeWidget>
#include <QSplitter>
#include <QBoxLayout>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QComboBox>
#include <QPushButton>
#include "Widgets/CollapsableWidget.h"
#include "EditorWindow.h"

SDK_REGISTER_WINDOW(CEditorSettingsWnd, "Editor Settings", "Edit", NULL);

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
	settingsView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

	QScrollArea* scrollArea = new QScrollArea(this);
	scrollArea->setAlignment(Qt::AlignTop | Qt::AlignHCenter); 
	scrollArea->setWidgetResizable(true);
	scrollArea->setWidget(settingsView);

	splitter->addWidget(scrollArea);

	splitter->setStretchFactor(0, 2);
	splitter->setStretchFactor(1, 6);

	{
		general = new QWidget(this);
		general->setLayout(new QVBoxLayout());
		settingsView->layout()->addWidget(general);

		general->layout()->addWidget(new QLabel("this is empty :))"));
	}
	{
		appearance = new QWidget(this);
		appearance->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
		auto* layout = new QVBoxLayout();
		appearance->setLayout(layout);
		settingsView->layout()->addWidget(appearance);
		appearance->hide();

		QHBoxLayout* themeLayout = new QHBoxLayout();
		layout->addLayout(themeLayout);
		
		CCollapsableWidget* themeWidget = new CCollapsableWidget("Theme", nullptr, this);
		themeWidget->SetHeaderType(CCollapsableWidget::TREE_HEADER);
		QComboBox* themeCombo = new QComboBox(this);
		themeCombo->addItem("default");

		for (auto& th : CEditorWindow::GetAvailableThemes())
			if (th != "default")
				themeCombo->addItem(th.c_str());

		themeCombo->setCurrentText(CEditorWindow::CurTheme().c_str());

		QPushButton* installButton = new QPushButton("Install Theme", this);

		QWidget* themeSettings = new QWidget(this);
		QVBoxLayout* themeSettingsLayout = new QVBoxLayout();
		themeSettings->setLayout(themeSettingsLayout);
		themeSettingsLayout->addWidget(new QLabel("No theme settings available.", this));

		themeWidget->SetWidget(themeSettings);

		themeLayout->addWidget(themeWidget);
		themeLayout->addStretch();
		themeLayout->addWidget(themeCombo);
		themeLayout->addWidget(installButton);

		connect(themeCombo, &QComboBox::currentTextChanged, this, [=](const QString& text) {
			gEditorWindow->SetTheme(text.toStdString().c_str());
		});
	}

	connect(settingsIndex, &QTreeWidget::itemSelectionChanged, this, [=]() {
		SwitchPage(settingsIndex->currentIndex().row());
	});

	RestoreState();
}

void CEditorSettingsWnd::SwitchPage(int index)
{
	if (index == curPage)
		return;

	QWidget* pages[] = {
		general,
		appearance
	};

	pages[curPage]->hide();
	curPage = index;

	pages[curPage]->show();
}

void CEditorSettingsWnd::UserSaveState(QSettings& out)
{
	out.setValue("splitter", splitter->saveState());
}

void CEditorSettingsWnd::UserRestoreState(QSettings& in)
{
	splitter->restoreState(in.value("splitter").toByteArray());
}
