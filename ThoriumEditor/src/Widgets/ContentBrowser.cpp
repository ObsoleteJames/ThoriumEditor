
#include "ContentBrowser.h"
#include "Assets/Asset.h"
#include "Engine.h"
//#include "Widgets/TreeDataItem.h"
#include "Console.h"
#include "Object/Class.h"
//#include "ImportDialogs/BaseImportDialog.h"
#include "FramelessDialog.h"
#include "EditorEngine.h"
#include "System.h"
#include "EditorConfig.h"

#include <QSplitter>
#include <QBoxLayout>
#include <QTreeWidget>
#include <QListWidget>
#include <QTableView>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>
#include <QSlider>
#include <QDropEvent>
#include <QProcess>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileIconProvider>
#include <QStandardItemModel>

#define ASSET_MAX_GRID_SIZE 5

CEditorVar evThumbnailMaxSize("Asset Thumbnail Max Size", "Appearance", FVariant(128));
CEditorVar evEnableThumbnails("Show Asset Thumbnails", "Appearance", FVariant(true));

FAssetBrowserAction::FAssetBrowserAction()
{
	_Actions().Add(this);
}

FAssetBrowserAction* FAssetBrowserAction::GetAction(FAssetClass* target, EBrowserActionType type /*= BA_INVALID*/)
{
	for (auto* action : _Actions())
	{
		if (action->targetClass == target)
		{
			if (type == BA_INVALID)
				return action;

			if (type == action->type)
				return action;
		}
	}

	return nullptr;
}

FAssetBrowserAction::FActionList& FAssetBrowserAction::_Actions()
{
	static FActionList actions;
	return actions;
}

class CAssetList : public QListWidget
{
public:
	CAssetList(QWidget* parent = nullptr) : QListWidget(parent) {}

protected:
	void dropEvent(QDropEvent* event) { }

};

class CFileItem : public QStandardItem
{
public:
	CFileItem(const QString& text, int type = QStandardItem::UserType) : QStandardItem(text), t(type) {}
	virtual int type() const override { return t; }

private:
	int t;
};

CAssetFilterMenu::CAssetFilterMenu(TArray<FClass*>* l, QWidget* parent) : QMenu(parent)
{
	filterList = l;
	
	TArray<FAssetClass*> classes;
	CModuleManager::GetAssetTypes(classes);

	QAction* clearFilter = new QAction("Clear Filters", this);
	connect(clearFilter, &QAction::triggered, this, [=]() {
		for (auto* a : actions)
			a->setChecked(false);
		filterList->Clear();
		emit(OnFilterUpdate());
	});
	addAction(clearFilter);
	addSeparator();

	for (auto* c : classes)
	{
		if (c->Flags() & CTAG_ABSTRACT || c->Flags() & CTAG_HIDDEN)
			continue;
		//QAction* action = new QAction(gEditorEngine()->GetResourceIcon(ToWString(c->GetExtension())), c->GetName().c_str());
		QAction* action = new QAction(c->GetName().c_str());
		action->setCheckable(true);
		connect(action, &QAction::triggered, this, [=](bool b) { this->SetFilter(c, b); });
		addAction(action);
		actions.Add(action);
	}
}

CAssetFilterMenu::~CAssetFilterMenu()
{
}

void CAssetFilterMenu::SetFilter(FClass* type, bool enabled)
{
	if (enabled)
	{
		if (filterList->Find(type) == filterList->end())
			filterList->Add(type);
	}
	else
	{
		auto it = filterList->Find(type);
		if (it != filterList->end())
			filterList->Erase(it);
	}

	emit(OnFilterUpdate());
}

CContentBrowserWidget::CContentBrowserWidget(QWidget* parent /*= nullptr*/) : QWidget(parent)
{
	setObjectName("assetbrowser_widget");

	QVBoxLayout* layout = new QVBoxLayout();
	setLayout(layout);

	QWidget* dirViewWidget = new QWidget(this);
	QVBoxLayout* dirViewLayout = new QVBoxLayout(dirViewWidget);
	QHBoxLayout* topLayout = new QHBoxLayout();

	QFrame* lineFrame = new QFrame(this);
	lineFrame->setLayout(topLayout);

	filterMenu = new CAssetFilterMenu(&activeFilters, this);

	btnRootFolder = new QPushButton(QIcon(":/icons/arrow-return.svg"), "", this);
	btnGoForward = new QPushButton(QIcon(":/icons/arrow-right.svg"), "", this);
	btnGoBack = new QPushButton(QIcon(":/icons/arrow-left.svg"), "", this);
	btnFilters = new QPushButton(QIcon(":/icons/filter-dropdown.svg"), "Filters", this);
	btnViewMode = new QPushButton(QIcon(":/icons/dropdown.svg"), "", this);
	btnCreateAsset = new QPushButton("+", this);
	curFolderEdit = new QLineEdit(this);

	gridSizeSlider = new QSlider(Qt::Horizontal, this);
	
	fileTree = new QTreeWidget(this);
	fileTree->setStyleSheet("QFrame { background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #242424, stop:1 #161616); }");
	//dirView = new CAssetList(this);
	//dirView->setProperty("type", QVariant(2));
	//dirView->setObjectName("CAssetBrowser::dirView");
	//dirView->setContextMenuPolicy(Qt::CustomContextMenu);
	//dirView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	//dirView->setStyleSheet("QFrame { background: #111; }");
	//connect(dirView, &QListWidget::customContextMenuRequested, this, &CContentBrowserWidget::CreateContextMenu);

	dirModel = new QStandardItemModel(0, 3);
	dirListView = new QListView(this);
	dirListView->setObjectName("CContentBrowser::dirListView");
	dirListView->setProperty("type", QVariant(2));
	dirListView->setContextMenuPolicy(Qt::CustomContextMenu);
	dirListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	dirListView->setDragDropMode(QListWidget::NoDragDrop);
	dirListView->setEditTriggers(QAbstractItemView::EditKeyPressed);
	dirListView->setModel(dirModel);

	dirTableView = new QTableView(this);
	dirTableView->setObjectName("CContentBrowser::dirTableView");
	dirTableView->setContextMenuPolicy(Qt::CustomContextMenu);
	dirTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	dirTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	dirTableView->setDragDropMode(QListWidget::NoDragDrop);
	dirTableView->setEditTriggers(QAbstractItemView::EditKeyPressed);
	dirTableView->setModel(dirModel);

	connect(dirListView, &QListView::customContextMenuRequested, this, &CContentBrowserWidget::CreateContextMenu);
	connect(dirTableView, &QListView::customContextMenuRequested, this, &CContentBrowserWidget::CreateContextMenu);

	connect(dirListView, &QListView::doubleClicked, this, &CContentBrowserWidget::dirDoubleClicked);
	connect(dirListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CContentBrowserWidget::dirSelectionChanged);

	connect(dirTableView, &QListView::doubleClicked, this, &CContentBrowserWidget::dirDoubleClicked);
	connect(dirTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CContentBrowserWidget::dirSelectionChanged);

	/*connect(dirView, &QListWidget::itemDoubleClicked, this, [=](QListWidgetItem* item) {
		if (item->type() == EItemTypes_Folder)
		{
			FString newDir = dir;
			if (*newDir.last() == L'/')
				newDir.Erase(newDir.last());

			if (newDir.IsEmpty())
				newDir = (const char*)item->text().toUtf8().constData();
			else
				newDir = newDir + "/" + (const char*)item->text().toUtf8().constData();

			SetDirectory(newDir);
		}
		else if (item->type() == EItemTypes_AssetFile)
		{
			FFile* file = (FFile*)item->data(257).toULongLong();
			selectedFile = file;
			emit(fileDoubleClicked());

			if (!file)
				return;

			FAssetClass* type = CAssetManager::GetAssetTypeByFile(file);

			if (auto* action = FAssetBrowserAction::GetAction(type, BA_FILE_OPEN); action)
			{
				FBADataBase data{ this, nullptr, file };
				action->Invoke(&data);
			}
		}
	});
	connect(dirView, &QListWidget::itemSelectionChanged, this, [=]() {
		selectedFiles.Clear();
		auto items = dirView->selectedItems();
		for (auto* item : items)
		{
			if (item->type() == EItemTypes_AssetFile)
			{
				selectedFiles.Add((FFile*)item->data(257).toULongLong());
			}
		}

		selectedFile = nullptr;
		if (dirView->selectedItems().size() == 0)
			return;

		auto* item = dirView->selectedItems()[0];
		if (item->type() == EItemTypes_AssetFile)
		{
			selectedFile = (FFile*)item->data(257).toULongLong();
		}
		emit(fileClicked());
	});*/

	connect(fileTree, &QTreeWidget::itemSelectionChanged, this, [=]() {
		if (fileTree->selectedItems().size() == 0)
			return;
	
		auto* item = fileTree->selectedItems()[0];

		if (item->type() == EItemTypes_Folder)
			SetDirectory((const char*)item->text(1).toUtf8().constData());
		else if (item->type() == EItemTypes_ModFolder)
			SetDirectory((const char*)item->text(0).toUtf8().constData(), FString());
	});

	//lineFrame->setProperty("type", QVariant(1));

	btnRootFolder->setProperty("type", QVariant("clear"));
	btnRootFolder->setMaximumSize(QSize(24, 24));
	btnRootFolder->setToolTip("Root Folder");
	btnGoForward->setEnabled(false);
	btnGoForward->setProperty("type", QVariant("clear"));
	btnGoForward->setMaximumSize(QSize(24, 24));
	btnGoForward->setToolTip("Forward");
	btnGoBack->setEnabled(false);
	btnGoBack->setProperty("type", QVariant("clear"));
	btnGoBack->setMaximumSize(QSize(24, 24));
	btnGoBack->setToolTip("Back");
	btnFilters->setProperty("type", QVariant("clear"));
	btnFilters->setMaximumSize(QSize(128, 24));
	btnViewMode->setProperty("type", QVariant("clear"));
	btnViewMode->setMaximumSize(QSize(24, 24));
	btnCreateAsset->setProperty("type", QVariant("clear"));
	btnCreateAsset->setMaximumSize(QSize(128, 24));

	gridSizeSlider->setMinimum(1);
	gridSizeSlider->setMaximum(ASSET_MAX_GRID_SIZE);
	gridSizeSlider->setMaximumSize(QSize(100, 24));
	gridSizeSlider->setValue(dirViewSize);

	connect(filterMenu, &CAssetFilterMenu::OnFilterUpdate, this, [=]() { UpdateView(); });
	connect(btnFilters, &QPushButton::clicked, this, [=]() { filterMenu->exec(btnFilters->mapToGlobal(QPoint(0, -filterMenu->sizeHint().height()))); });
	connect(curFolderEdit, &QLineEdit::editingFinished, this, [=]() {
		FString m, d;
		ExtractPath(curFolderEdit->text().toUtf8().constData(), m, d);
		SetDirectory(m, d);
	});
	connect(btnGoBack, &QPushButton::clicked, this, [=]() {
		if (historyIndex < dirHistory.Size())
			historyIndex++;

		if (historyIndex >= dirHistory.Size())
			btnGoBack->setEnabled(false);

		FString m, d;
		ExtractPath(dirHistory[dirHistory.Size() - historyIndex], m, d);
		SetDirectory(m, d, true);
	});
	connect(btnGoForward, &QPushButton::clicked, this, [=]() {
		if (historyIndex > 0)
			historyIndex--;

		if (historyIndex == 0)
			btnGoForward->setEnabled(false);

		FString m, d;
		ExtractPath(dirHistory[dirHistory.Size() - historyIndex + 1], m, d);
		SetDirectory(m, d, true);
	});
	connect(btnRootFolder, &QPushButton::clicked, this, [=]() { 
		FString newDir = dir;
		SizeType it = newDir.FindLastOf("\\/");
		if (it != -1)
			newDir.Erase(newDir.begin() + it, newDir.end());
		else
			newDir.Clear();

		SetDirectory(newDir);
	});
	connect(btnViewMode, &QPushButton::clicked, this, [=]() { bDirViewGrid ^= 1; UpdateViewSettings(); });
	connect(gridSizeSlider, &QSlider::valueChanged, this, [=](int value) { dirViewSize = value; UpdateViewSettings(); });
	/*connect(btnCreateAsset, &QPushButton::clicked, this, [=]() { 
		QMenu menu;

		for (auto& m : assetMenus)
		{
			menu.addAction(m.name, this, [=]() { m.func(curDir); UpdateView(); });
		}
		menu.exec(btnCreateAsset->mapToGlobal(QPoint(0, -btnCreateAsset->sizeHint().height())));
	});*/

	topLayout->addWidget(btnRootFolder);
	topLayout->addWidget(btnGoBack);
	topLayout->addWidget(btnGoForward);
	topLayout->addWidget(btnFilters);
	topLayout->addWidget(btnCreateAsset);
	topLayout->addWidget(curFolderEdit);
	topLayout->addWidget(btnViewMode);
	topLayout->addWidget(gridSizeSlider);

	fileTree->setHeaderHidden(true);

	splitter = new QSplitter(this);
	splitter->addWidget(fileTree);
	splitter->setStretchFactor(0, 2);
	splitter->addWidget(dirViewWidget);
	splitter->setStretchFactor(1, 6);

	dirViewLayout->setContentsMargins(0, 0, 0, 0);
	dirViewLayout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	topLayout->setSpacing(2);
	topLayout->setContentsMargins(2, 2, 2, 2);
	dirViewLayout->addWidget(lineFrame);
	//dirViewLayout->addWidget(dirView);
	dirViewLayout->addWidget(dirListView);
	dirViewLayout->addWidget(dirTableView);
	layout->addWidget(splitter);

	dirTableView->hide();

	dir = "";

	if (gEngine->ActiveGame().name.IsEmpty())
		mod = "Engine";
	else
		mod = gEngine->ActiveGame().name;
	
	UpdateViewSettings();

	OnAssetUpdate();
}

CContentBrowserWidget::~CContentBrowserWidget()
{
}

void CContentBrowserWidget::dirDoubleClicked(const QModelIndex& index)
{
	auto* item = dirModel->item(index.row());

	if (item->type() == EItemTypes_Folder)
	{
		FString newDir = dir;
		if (*newDir.last() == L'/')
			newDir.Erase(newDir.last());

		if (newDir.IsEmpty())
			newDir = (const char*)item->text().toUtf8().constData();
		else
			newDir = newDir + "/" + (const char*)item->text().toUtf8().constData();

		SetDirectory(newDir);
	}
	else if (item->type() == EItemTypes_AssetFile)
	{
		FFile* file = (FFile*)item->data(257).toULongLong();
		selectedFile = file;
		emit(fileDoubleClicked());

		if (!file || !bCreateFiles)
			return;
		
		FAssetClass* type = CAssetManager::GetAssetTypeByFile(file);

		if (auto* action = FAssetBrowserAction::GetAction(type, BA_FILE_OPEN); action)
		{
			FBADataBase data{ this, nullptr, file };
			action->Invoke(&data);
		}
	}
}

void CContentBrowserWidget::dirSelectionChanged(const QItemSelection& selected, const QItemSelection& dselected)
{
	selectedFiles.Clear();
	//auto items = dirModel->selectedItems();
	//auto items = selected;

	for (auto& range : selected)
	{
		for (auto& index : range.indexes())
		{
			auto* item = dirModel->item(index.row());
			if (item->type() == EItemTypes_AssetFile)
			{
				selectedFiles.Add((FFile*)item->data(257).toULongLong());
			}
		}
	}

	selectedFile = nullptr;
	if (selectedFiles.Size() == 0)
		return;

	selectedFile = selectedFiles[0];
	//auto* item = dirView->selectedItems()[0];
	//if (item->type() == EItemTypes_AssetFile)
	//{
	//	selectedFile = (FFile*)item->data(257).toULongLong();
	//}
	emit(fileClicked());
}

void CContentBrowserWidget::SetDirectory(const FString& d, bool fromHistory)
{
	if (GetFDirectory(mod, d) == nullptr)
	{
		curFolderEdit->setText((mod + ":/" + dir).c_str());
		return;
	}

	if (historyIndex > 0)
	{
		if (!fromHistory)
		{
			SizeType i = dirHistory.Size() - historyIndex;
			dirHistory.Erase(dirHistory.begin() + i, dirHistory.end());
			historyIndex = 0;
			btnGoForward->setEnabled(false);
		}
		else
			btnGoForward->setEnabled(true);
	}

	if (!fromHistory)
	{
		if (dirHistory.Size() == 16)
			dirHistory.Erase(dirHistory.begin());

		btnGoBack->setEnabled(true);
		dirHistory.Add(mod + ":/" + dir);
	}

	dir = d;
	UpdateView();
}

void CContentBrowserWidget::SetDirectory(const FString& m, const FString& d, bool fromHistory)
{
	if (GetFDirectory(m, d) == nullptr)
	{
		curFolderEdit->setText((mod + ":/" + dir).c_str());
		return;
	}

	if (historyIndex > 0)
	{
		if (!fromHistory)
		{
			SizeType i = dirHistory.Size() - historyIndex;
			dirHistory.Erase(dirHistory.begin() + i, dirHistory.end());
			historyIndex = 0;
			btnGoForward->setEnabled(false);
		}
		else
			btnGoForward->setEnabled(true);
	}

	if (!fromHistory)
	{
		if (dirHistory.Size() == 16)
			dirHistory.Erase(dirHistory.begin());

		btnGoBack->setEnabled(true);
		dirHistory.Add(mod + ":/" + dir);
	}

	dir = d;
	mod = m;
	UpdateView();
}

FDirectory* CContentBrowserWidget::GetFDirectory(const FString& path)
{
	FString mod = path;
	FString dir = path;

	if (auto it = mod.FindFirstOf(':'); mod.begin() + it != mod.end())
	{
		mod.Erase(mod.begin() + it, mod.end());

		if (it + 2 < dir.Size())
			dir.Erase(dir.begin(), dir.begin() + it + 2);
		else
			dir.Clear();
	}
	else
		return nullptr;

	FMod* m = CFileSystem::FindMod(mod);
	if (m == nullptr)
		return nullptr;

	if (dir.IsEmpty())
		return m->GetRootDir();

	if (FDirectory* d = m->FindDirectory(dir); d != nullptr)
		return d;

	return nullptr;
}

FDirectory* CContentBrowserWidget::GetFDirectory(const FString& mod, const FString& path)
{
	FMod* m = CFileSystem::FindMod(mod);
	if (m == nullptr)
		return nullptr;

	if (path.IsEmpty())
		return m->GetRootDir();

	if (FDirectory* d = m->FindDirectory(path); d != nullptr)
		return d;

	return nullptr;
}

void CContentBrowserWidget::AddDirToTree(FMod* mod, FDirectory* dir, QTreeWidgetItem* parent)
{
	QTreeWidgetItem* item = new QTreeWidgetItem(parent, EItemTypes_Folder);
	item->setText(0, QString(dir->GetName().c_str()));
	FString path = dir->GetPath();
	item->setText(1, QString(path.c_str()));
	item->setIcon(0, QIcon(":/icons/folder-small.svg"));

	for (auto* d : dir->GetSubDirectories())
		AddDirToTree(mod, d, item);
}

void CContentBrowserWidget::CreateContextMenu(QPoint point)
{
	//auto* item = dirView->itemAt(point);
	auto index = dirListView->indexAt(point);
	auto* item = dirModel->item(index.row());

	QMenu menu(this);

	if (item)
	{
		if (item->type() == EItemTypes_AssetFile)
		{
			FFile* f = (FFile*)item->data(257).toULongLong();
			FAssetClass* type = CAssetManager::GetAssetTypeByFile(f);

			if (!type)
			{
				menu.addAction("Conver to Asset...");
				menu.addSeparator();
			}

			if (auto* action = FAssetBrowserAction::GetAction(type, BA_FILE_CONTEXTMENU); action)
			{
				FBADataBase data{ this, &menu, f };
				action->Invoke(&data);

				menu.addSeparator();
			}

			menu.addAction("Copy");
			menu.addAction("Duplicate...");
			menu.addAction("Rename...");
			menu.addAction("Copy Path", this, [=]() { SSystem::SetClipboardData(f->FullPath()); });
			
			if (type)
			{
				menu.addAction("Copy ID", this, [=]() {
					auto* data = CAssetManager::GetAssetData(f->Path());
					SSystem::SetClipboardData(FString::ToString(data->id));
				});
			}
		}

		menu.addAction("Delete", this, [=]() {
			QMessageBox msg;
			msg.setText("Are you sure you want to delete '" + item->text() + "'? this cannot be undone!");
			msg.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
			msg.setDefaultButton(QMessageBox::Cancel);
			msg.setIcon(QMessageBox::Warning);

			int r = msg.exec();
			if (r != QMessageBox::Yes)
				return;

			if (item->type() == EItemTypes_AssetFile)
			{
				FFile* f = (FFile*)item->data(257).toULongLong();
				f->Mod()->DeleteFile(f->Path());
			}
			else if (item->type() == EItemTypes_Folder)
			{
				FDirectory* d = (FDirectory*)item->data(257).toULongLong();
				CFileSystem::FindMod(mod)->DeleteDirectory(d->GetPath());
			}
			UpdateView();
		});

		if (item->type() == EItemTypes_AssetFile)
		{
			menu.addSeparator();

			menu.addAction("Show in explorer", this, [=]() {
				FFile* f = (FFile*)item->data(257).toULongLong();
				SSystem::OpenFileManager(f->Mod()->Path() + "/" + f->Dir()->GetPath());
			});

			menu.addAction("Open in External Program", this, [=]() {
				FFile* f = (FFile*)item->data(257).toULongLong();
				SSystem::OpenFile(f->FullPath());
			});

			if (FFile* file = (FFile*)item->data(257).toULongLong(); file && file->Extension() == ".thcs")
			{
				CShaderSource* shader = CAssetManager::GetAsset<CShaderSource>(file->Path());
				menu.addAction("Compile", this, [=]() { shader->Compile(); });
			}
		}
	}
	else
	{
		menu.addAction("New Folder", this, [=]() { 
			CFramelessDialog* dialog = new CFramelessDialog(this);

			QFrame* frame = new QFrame(dialog);
			QVBoxLayout* layout = new QVBoxLayout(frame);

			dialog->setCentralWidget(frame);
			dialog->setTitle("New Folder");

			QLineEdit* nameEdit = new QLineEdit(this);
			nameEdit->setPlaceholderText("Name...");

			layout->addWidget(nameEdit);

			QHBoxLayout* l1 = new QHBoxLayout();

			QPushButton* btnImport = new QPushButton("Create", frame);
			btnImport->setProperty("type", QVariant("primary"));
			btnImport->setEnabled(false);
			QPushButton* btnCancel = new QPushButton("Cancel", frame);

			l1->addWidget(btnImport);
			l1->addWidget(btnCancel);

			layout->addLayout(l1);

			connect(nameEdit, &QLineEdit::textChanged, this, [=](const QString& str) { btnImport->setEnabled(!str.isEmpty()); });
			connect(nameEdit, &QLineEdit::returnPressed, this, [=]() { dialog->done(true); });

			connect(btnImport, &QPushButton::clicked, this, [=]() { dialog->done(true); });
			connect(btnCancel, &QPushButton::clicked, this, [=]() { dialog->done(false); });

			if (dialog->exec() && !nameEdit->text().isEmpty())
			{
				CFileSystem::FindMod(mod)->CreateDir(dir + "/" + nameEdit->text().toStdString());
				UpdateView();
			}
		});

		if (bCreateFiles)
		{
			menu.addSeparator();

			//for (auto& m : assetMenus)
			//	menu.addAction(m.name, this, [=]() { m.func(curDir); });

			for (auto* action : FAssetBrowserAction::GetActions())
			{
				if (action->Type() == BA_WINDOW_CONTEXTMENU)
				{
					FBAWindowContext data{ this, &menu, nullptr, mod, dir };
					action->Invoke(&data);
				}
			}

			menu.addSeparator();
			menu.addAction("Import Asset", this, [=]() { ImportAsset(); });
		}
	}

	menu.exec(QCursor::pos());
}

void CContentBrowserWidget::ImportAsset()
{
	FString filter;
	TArray<FAssetClass*> importableClasses;
	for (CModule* m : CModuleManager::GetModules())
	{
		for (FAssetClass* c : m->Assets)
		{
			if (c->ImportableAs().IsEmpty())
				continue;

			TArray<FString> imports = c->ImportableAs().Split(';');

			filter += c->GetName();
			filter += " (";

			for (auto& i : imports)
				filter += "*" + i + " ";

			filter.Erase(filter.last());
			filter += ");;";
			importableClasses.Add(c);
		}
	}

	filter += "All Files (*.*)";

	QStringList file = QFileDialog::getOpenFileNames(this, "Select File...", QString(), filter.c_str());
	if (file.isEmpty())
		return;

	FString ext = file[0].toStdString();
	ext.Erase(ext.begin(), ext.begin() + ext.FindLastOf('.'));

	FAssetClass* targetClass = nullptr;

	// Figure out what the selected file's type is.
	for (auto* c : importableClasses)
	{
		TArray<FString> exts = c->ImportableAs().Split(';');

		for (auto& x : exts)
		{
			if (x == ext)
			{
				targetClass = c;
				break;
			}
		}

		if (targetClass)
			break;
	}

	if (!targetClass)
		return;

	/*FString dir = curDir;
	FString mod = curDir;

	if (auto it = mod.FindFirstOf(':'); mod.begin() + it != mod.end())
	{
		mod.Erase(mod.begin() + it, mod.end());

		if (it + 2 < dir.Size())
			dir.Erase(dir.begin(), dir.begin() + it + 2);
		else
			dir.Clear();
	}*/

	//if (IImportDialog* dialog = IImportDialog::GetImportDialog(targetClass, this); dialog != nullptr)
	//{
	//	dialog->Exec(file, dir, mod);
	//	delete dialog;
	//}
	//else
	//{

	//	FString fileName = file[0].toStdString();
	//	fileName.Erase(fileName.begin() + fileName.FindLastOf('.'), fileName.end());
	//	if (auto it = fileName.FindLastOf("/\\"); it != -1)
	//		fileName.Erase(fileName.begin(), fileName.begin() + it + 1);

	//	TObjectPtr<CAsset> asset = CResourceManager::CreateResource(targetClass, dir + L"\\" + ToWString(fileName) + ToWString(targetClass->GetExtension()), mod);
	//	asset->Import(file[0].toStdWString());
	//}

	UpdateView();
}

bool CContentBrowserWidget::ExtractPath(const FString& combined, FString& outMod, FString& outDir)
{
	SizeType colon = combined.FindFirstOf(':');
	if (colon != -1)
	{
		outDir = combined;
		outMod = combined;
		SizeType i = outDir.Size() > colon + 1 ? 2 : 1;
		outDir.Erase(outDir.begin(), outDir.begin() + colon + i);
		outMod.Erase(outMod.begin() + colon, outMod.end());
		return true;
	}
	return false;
}

void CContentBrowserWidget::SetGridSize(int size)
{
	dirViewSize = FMath::Clamp(size, 1, ASSET_MAX_GRID_SIZE);
	gridSizeSlider->setValue(dirViewSize);
	UpdateViewSettings();
}

void CContentBrowserWidget::LockAssetFilter()
{
	bFiltersLocked = true; 
	btnFilters->setDisabled(true);
}

void CContentBrowserWidget::OnAssetUpdate()
{
	fileTree->clear();

	const TArray<FMod*>& mods = CFileSystem::GetMods();
	for (auto* m : mods)
	{
		QTreeWidgetItem* item = new QTreeWidgetItem(fileTree, EItemTypes_ModFolder);
		item->setText(0, QString(m->Name().c_str()));

		if (m->Name() == "Engine")
			item->setIcon(0, QIcon(":/icons/engine-icon-small.svg"));
		else
			item->setIcon(0, QIcon(":/icons/folder-blue.svg"));

		//item->setIcon(0, gEditorEngine()->GetIcon("folder-blue.svg"));
		item->setExpanded(true);

		for (auto* d : m->GetRootDir()->GetSubDirectories())
			AddDirToTree(m, d, item);
	}

	UpdateView();
}

void CContentBrowserWidget::UpdateView()
{
	curFolderEdit->setText((mod + ":/" + dir).c_str());

	dirModel->clear();

	FDirectory* dir = GetFDirectory(mod, this->dir);
	if (!dir)
		return;

	for (auto* d : dir->GetSubDirectories())
	{
		CFileItem* item = new CFileItem(QString(d->GetName().c_str()), EItemTypes_Folder);
		//QListWidgetItem* item = new QListWidgetItem(QString(d->GetName().c_str()), dirView, EItemTypes_Folder);
		item->setData(QVariant((SizeType)d), 257);
		item->setIcon(QIcon(":/icons/folder.svg"));
		dirModel->appendRow({ item, new QStandardItem("Folder"), new QStandardItem("") });
	}

	for (auto* f : dir->GetFiles())
	{
		FAssetClass* type = CAssetManager::GetAssetTypeByFile(f);
		if (activeFilters.Size() > 0)
		{
			auto it = activeFilters.Find(type);
			if (it == activeFilters.end())
				continue;
		}

		//QListWidgetItem* item = new QListWidgetItem(QString(f->Name().c_str()), dirView, EItemTypes_AssetFile);
		CFileItem* item = new CFileItem(QString(f->Name().c_str()), EItemTypes_AssetFile);

		item->setData(QVariant((SizeType)f));
		item->setIcon(QIcon(":/icons/file.svg"));

		if (!type)
			item->setIcon(QFileIconProvider().icon(QFileInfo(f->FullPath().c_str())));

		FString tooltip = "Type: " + f->Extension() + "\nSize: " + FString::ToString(f->Size());
		item->setToolTip(QString(tooltip.c_str()));

		dirModel->appendRow({ item, new QStandardItem(type ? type->GetName().c_str() : f->Extension().c_str()), new QStandardItem(QString::number(f->Size()))});
	}

	dirModel->setHeaderData(0, Qt::Horizontal, "Name");
	dirModel->setHeaderData(1, Qt::Horizontal, "Type");
	dirModel->setHeaderData(2, Qt::Horizontal, "Size");
}

void CContentBrowserWidget::UpdateViewSettings()
{
	if (bDirViewGrid)
	{
		btnViewMode->setIcon(QIcon(":/icons/grid-view.svg"));
		gridSizeSlider->setEnabled(true);

		dirListView->show();
		dirTableView->hide();

		dirListView->setFlow(QListView::LeftToRight);
		dirListView->setResizeMode(QListView::Adjust);
		dirListView->setGridSize(QSize((24 * dirViewSize) + 20, (30 * dirViewSize) + 20));
		dirListView->setIconSize(QSize(24 * dirViewSize, 24 * dirViewSize));
		//dirView->setSpacing(2);
		dirListView->setViewMode(QListView::IconMode);
	}
	else
	{
		btnViewMode->setIcon(QIcon(":/icons/dropdown.svg"));
		gridSizeSlider->setEnabled(false);

		dirListView->hide();
		dirTableView->show();
		//dirView->setFlow(QListView::TopToBottom);
		//dirView->setResizeMode(QListView::Fixed);
		////dirView->setSpacing(2);
		//dirView->setGridSize(QSize(-1, -1));
		//dirView->setIconSize(QSize(-1, -1));
		//dirView->setViewMode(QListView::ListMode);
	}
}
