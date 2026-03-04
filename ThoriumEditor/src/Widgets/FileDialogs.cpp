
#include <string>

#include "FileDialogs.h"
#include "ContentBrowser.h"
#include "Object/Class.h"

#include <QSpacerItem>
#include <QBoxLayout>
#include <QPushButton>
#include <QLineEdit>

CSaveFileDialog::CSaveFileDialog(QWidget* parent /*= nullptr*/) : CFramelessDialog(parent)
{
	QFrame* frame = new QFrame(this);
	QVBoxLayout* layout = new QVBoxLayout(frame);

	//{
	//	QVBoxLayout* l = new QVBoxLayout(this);
	//	l->setContentsMargins(0, 0, 0, 0);
	//	l->addWidget(frame);
	//}
	
	setCentralWidget(frame);
	setTitle("Save Asset...");

	assetBrowser = new CContentBrowserWidget(this);
	assetBrowser->DisableFileCreation();
	
	QHBoxLayout* l2 = new QHBoxLayout();

	nameEdit = new QLineEdit(this);
	nameEdit->setPlaceholderText("file name...");

	QPushButton* btnSave = new QPushButton("Save", this);
	QPushButton* btnCancel = new QPushButton("Cancel", this);

	btnSave->setDisabled(true);
	btnSave->setProperty("type", QVariant("primary"));

	l2->addWidget(nameEdit);
	l2->addWidget(btnSave);
	l2->addWidget(btnCancel);

	layout->addWidget(assetBrowser);
	layout->addLayout(l2);

	resize(850, 560);

	connect(nameEdit, &QLineEdit::textEdited, this, [=](const QString& txt) { btnSave->setDisabled(txt.isEmpty()); });
	connect(btnSave, &QPushButton::clicked, this, &CSaveFileDialog::Save);
	connect(btnCancel, &QPushButton::clicked, this, [=]() { done(0); deleteLater(); });
	connect(nameEdit, &QLineEdit::returnPressed, this, &CSaveFileDialog::Save);
}

void CSaveFileDialog::Save()
{
	if (nameEdit->text().isEmpty())
		return;

	path = assetBrowser->GetDirectory();
	if (*path.last() != '\\')
		path += "\\";
	
	path += nameEdit->text().toStdString();
	mod = assetBrowser->GetMod();

	done(1);
	deleteLater();
}

//COpenFileDialog::COpenFileDialog(const FString& filter, QWidget* parent /*= nullptr*/) : CFramelessDialog(parent), file(nullptr)
//{
//	QFrame* frame = new QFrame(this);
//	QVBoxLayout* layout = new QVBoxLayout(frame);
//
//	//{
//	//	QVBoxLayout* l = new QVBoxLayout(this);
//	//	l->setContentsMargins(0, 0, 0, 0);
//	//	l->addWidget(frame);
//	//}
//
//	setCentralWidget(frame);
//	setTitle("Open Asset...");
//
//	assetBrowser = new CContentBrowserWidget(this);
//	assetBrowser->DisableFileCreation();
//	assetBrowser->AddAssetFilter(filter);
//	assetBrowser->LockAssetFilter();
//
//	QHBoxLayout* l2 = new QHBoxLayout();
//
//	QPushButton* btnSelect = new QPushButton("Select", this);
//	QPushButton* btnCancel = new QPushButton("Cancel", this);
//
//	btnSelect->setDisabled(true);
//	btnSelect->setProperty("type", QVariant("primary"));
//
//	l2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
//	l2->addWidget(btnSelect);
//	l2->addWidget(btnCancel);
//
//	layout->addWidget(assetBrowser);
//	layout->addLayout(l2);
//
//	resize(850, 560);
//
//	connect(assetBrowser, &CContentBrowserWidget::fileClicked, this, [=]() { file = assetBrowser->SelectedFile(); btnSelect->setDisabled(file == nullptr); });
//	connect(assetBrowser, &CContentBrowserWidget::fileDoubleClicked, this, [=]() { file = assetBrowser->SelectedFile(); done(1); deleteLater(); });
//	connect(btnSelect, &QPushButton::clicked, this, [=]() { done(1); deleteLater(); });
//	connect(btnCancel, &QPushButton::clicked, this, [=]() { done(0); deleteLater(); });
//}

COpenFileDialog::COpenFileDialog(FAssetClass* filterType, QWidget* parent /*= nullptr*/) : CFramelessDialog(parent), file(nullptr)
{
	QFrame* frame = new QFrame(this);
	QVBoxLayout* layout = new QVBoxLayout(frame);

	//{
	//	QVBoxLayout* l = new QVBoxLayout(this);
	//	l->setContentsMargins(0, 0, 0, 0);
	//	l->addWidget(frame);
	//}

	setCentralWidget(frame);
	setTitle(QString("Open ") + filterType->GetName().c_str() + "...");

	assetBrowser = new CContentBrowserWidget(this);
	assetBrowser->DisableFileCreation();
	if (filterType)
	{
		assetBrowser->AddAssetFilter(filterType);

		TArray<FClass*> children;
		CModuleManager::FindChildClasses(filterType, children);
		
		for (auto* c : children)
			assetBrowser->AddAssetFilter(filterType);
	}
	assetBrowser->LockAssetFilter();

	QHBoxLayout* l2 = new QHBoxLayout();

	QPushButton* btnSelect = new QPushButton("Select", this);
	QPushButton* btnCancel = new QPushButton("Cancel", this);

	btnSelect->setDisabled(true);
	btnSelect->setProperty("type", QVariant("primary"));

	l2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
	l2->addWidget(btnSelect);
	l2->addWidget(btnCancel);

	layout->addWidget(assetBrowser);
	layout->addLayout(l2);

	resize(850, 560);

	connect(assetBrowser, &CContentBrowserWidget::fileClicked, this, [=]() { file = assetBrowser->SelectedFile(); btnSelect->setDisabled(file == nullptr); });
	connect(assetBrowser, &CContentBrowserWidget::fileDoubleClicked, this, [=]() { file = assetBrowser->SelectedFile(); done(1); deleteLater(); });
	connect(btnSelect, &QPushButton::clicked, this, [=]() { done(1); deleteLater(); });
	connect(btnCancel, &QPushButton::clicked, this, [=]() { done(0); deleteLater(); });
}
