
#include "EditorSettings.h"

#include <QListWidget>
#include <QSplitter>
#include <QBoxLayout>
#include <QGridLayout>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QComboBox>
#include <QPushButton>
#include <QStackedWidget>
#include "Widgets/CollapsableWidget.h"
#include "EditorWindow.h"

#include "QtColorWidgets/color_selector.hpp"

SDK_REGISTER_WINDOW(CEditorSettingsWnd, "Editor Settings", "Edit", NULL);

class CSettingsPage : public QWidget
{
public:
	CSettingsPage(QWidget* parent) : QWidget(parent)
	{
		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);

		auto* scroll = new QScrollArea(this);
		scroll->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
		scroll->setWidgetResizable(true);
		scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		layout->addWidget(scroll);

		page = new QWidget(this);
		pageLayout = new QVBoxLayout(page);
		page->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
		scroll->setWidget(page);
	}

	inline void AddSetting(QWidget* item)
	{
		pageLayout->addWidget(item);

		QFrame* f = new QFrame(this);
		f->setFrameShape(QFrame::HLine);
		f->setLineWidth(1);
		pageLayout->addWidget(f);
	}

	QWidget* page;
	QVBoxLayout* pageLayout;
	QString title;
};

void CEditorSettingsWnd::SetupUi()
{
	auto r = geometry();
	r.setSize({ 840, 520 });
	setGeometry(r);

	QWidget* widget = new QWidget(this);
	setCentralWidget(widget);

	QGridLayout* layout = new QGridLayout(widget);
	widget->setLayout(layout);

	splitter = new QSplitter(Qt::Horizontal, this);
	layout->addWidget(splitter, 0, 0, 1, 4);

	settingsIndex = new QListWidget(this);
	splitter->addWidget(settingsIndex);

	//(new QListWidgetItem(settingsIndex))->setText("General");
	//(new QListWidgetItem(settingsIndex))->setText("Appearance");
	//(new QListWidgetItem(settingsIndex))->setText("User");
	//(new QListWidgetItem(settingsIndex))->setText("Source Code");

	settingsView = new QStackedWidget(this);

	QScrollArea* scrollArea = new QScrollArea(this);
	scrollArea->setAlignment(Qt::AlignTop | Qt::AlignHCenter); 
	scrollArea->setWidgetResizable(true);
	scrollArea->setWidget(settingsView);

	splitter->addWidget(scrollArea);

	splitter->setStretchFactor(0, 2);
	splitter->setStretchFactor(1, 6);

	{
		general = GetPage("General");
		general->AddSetting(new QLabel("Hello!"));
	}
	{
		appearance = GetPage("Appearance");

		QHBoxLayout* themeLayout = new QHBoxLayout();
		appearance->pageLayout->addLayout(themeLayout);
		
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

	auto& vars = CEditorVar::GetVariables();
	for (auto& v : vars)
	{
		if (!v->ShowInSettings())
			continue;

		auto* page = GetPage(v->GetGroup().c_str());

		QWidget* editor = nullptr;
		switch (v->GetValue().Type())
		{
		case FVariant::COLOR:
		{
			auto* edit = new color_widgets::ColorSelector(this);
			FColor col = v->GetValue().AsColor();
			edit->setColor(QColor(col.r * 255, col.g * 255, col.b * 255, col.a * 255));
			edit->setMinimumWidth(240);
			editor = edit;

			connect(edit, &color_widgets::ColorSelector::colorSelected, this, [=]() { 
				auto c = edit->color();
				v->SetValue(FVariant(FColor(c.redF(), c.greenF(), c.blueF(), c.alphaF()))); 
			});
		}
			break;
		}

		QWidget* setting = new QWidget(this);
		QHBoxLayout* l = new QHBoxLayout(setting);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(new QLabel(v->GetName().c_str()));
		l->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
		l->addWidget(editor);

		QPushButton* btn = new QPushButton(QIcon(":/icons/field_revert.svg"), "", this);
		btn->setToolTip("Revert to default value");
		btn->setProperty("type", QVariant("clear"));
		QSizePolicy sp = btn->sizePolicy(); sp.setRetainSizeWhenHidden(true);
		btn->setSizePolicy(sp);
		btn->setMaximumSize(20, 20);
		l->addWidget(btn);

		connect(btn, &QPushButton::clicked, this, [=]() { v->Revert(); });
		page->AddSetting(setting);
	}

	connect(settingsIndex, &QListWidget::currentRowChanged, settingsView, &QStackedWidget::setCurrentIndex);
	RestoreState();
}

void CEditorSettingsWnd::SwitchPage(int index)
{
	settingsView->setCurrentIndex(index);
}

void CEditorSettingsWnd::UserSaveState(QSettings& out)
{
	out.setValue("splitter", splitter->saveState());
}

void CEditorSettingsWnd::UserRestoreState(QSettings& in)
{
	splitter->restoreState(in.value("splitter").toByteArray());
}

void CEditorSettingsWnd::AddPage(QWidget* page, const QString& title)
{
	settingsView->addWidget(page);
	settingsIndex->addItem(title);

	//pages.Add(page);
}

CSettingsPage* CEditorSettingsWnd::GetPage(const QString& title, bool bCreateNew)
{
	for (auto* p : pages)
		if (p->title == title)
			return p;

	if (bCreateNew)
	{
		auto* p = new CSettingsPage(this);
		p->title = title;
		pages.Add(p);

		settingsView->addWidget(p);
		settingsIndex->addItem(title);

		return p;
	}

	return nullptr;
}
