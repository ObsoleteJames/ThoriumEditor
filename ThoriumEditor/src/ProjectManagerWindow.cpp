
#include <string>
#include "Engine.h"
#include "EditorEngine.h"
#include "ProjectManagerWindow.h"
#include "Misc/FileHelper.h"
#include "EditorWindow.h"
#include <Util/KeyValue.h>
#include "System.h"

#include "Widgets/FramelessDialog.h"
#include <filesystem>
#include <QPainter>
#include <QPushButton>
#include <QBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QFileDialog>
#include <QLineEdit>
#include <QStandardPaths>
#include <QCheckBox>

SDK_REGISTER_WINDOW(CProjectManagerWnd, "Project Manager", NULL, NULL);

CProjectManagerWnd::CProjectManagerWnd()
{
	SearchForProjects();
}

CProjectManagerWnd::~CProjectManagerWnd()
{
	SaveState();
}

bool CProjectManagerWnd::Shutdown()
{
	return true;
}

void CProjectManagerWnd::SetupUi()
{
	CToolsWindow::SetupUi();

	setWindowTitle("Project Manager");

	QWidget* widget = new QWidget(this);
	setCentralWidget(widget);

	int x = QGuiApplication::primaryScreen()->geometry().width();
	int y = QGuiApplication::primaryScreen()->geometry().height();

	x -= 800;
	y -= 500;
	x /= 2;
	y /= 2;
	setGeometry(QRect(x, y, 800, 500));

	QHBoxLayout* layout = new QHBoxLayout(widget);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	widget->setLayout(layout);

	QFrame* frame = new QFrame(this);
	QVBoxLayout* layout3 = new QVBoxLayout(frame);
	frame->setLayout(layout3);
	QPushButton* btn1 = new QPushButton("Continue without project...", this);

	layout3->addWidget(btn1);
	layout3->addStretch(1);

	QCheckBox* btnShowStartup = new QCheckBox("Show at startup", this);
	btnShowStartup->setChecked(bShowStartup);
	layout3->addWidget(btnShowStartup);

	layout->addWidget(frame);

	QVBoxLayout* layout2 = new QVBoxLayout(this);
	layout->addLayout(layout2);
	QHBoxLayout* topBar = new QHBoxLayout(this);
	topBar->setContentsMargins(8, 8, 8, 8);
	layout2->setContentsMargins(8, 8, 8, 8);
	layout2->setSpacing(5);
	layout2->addLayout(topBar);

	QLabel* lbl = new QLabel("Projects", this);
	QFont font = lbl->font();
	font.setPointSize(16);
	font.setBold(true);
	font.setLetterSpacing(QFont::AbsoluteSpacing, 2.f);
	lbl->setFont(font);
	topBar->addWidget(lbl);

	QSpacerItem* spacer = new QSpacerItem(1, 0, QSizePolicy::Expanding);
	topBar->addItem(spacer);

	QPushButton* openProjBtn = new QPushButton("Open Project", this);
	topBar->addWidget(openProjBtn);

	QPushButton* createProjBtn = new QPushButton("New Project", this);
	createProjBtn->setProperty("type", QVariant("primary"));
	topBar->addWidget(createProjBtn);

	projectList = new QListWidget(this);
	projectList->setFlow(QListView::TopToBottom);
	projectList->setResizeMode(QListView::Adjust);
	projectList->setGridSize(QSize(92, 74));
	projectList->setIconSize(QSize(72, 72));
	projectList->setSpacing(0);
	projectList->setViewMode(QListView::ListMode);
	//projectList->setStyleSheet("QListView { background: #111; border: 1px solid #1A1A1A; }");
	layout2->addWidget(projectList);

	connect(projectList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
		int proj = item->data(Qt::UserRole + 1).toInt();
		//if (!gEngine->LoadProject(projects[proj].path))
		//	return;
		if (!StartEngineThread(projects[proj].path))
		{
			DestroyEngineThread();
			return;
		}

		CToolsWindow::Create<CEditorWindow>();

		close();
		deleteLater();
	});
	connect(createProjBtn, &QPushButton::clicked, this, &CProjectManagerWnd::CreateNewProject);
	connect(openProjBtn, &QPushButton::clicked, this, &CProjectManagerWnd::OpenProject);
	connect(btn1, &QPushButton::clicked, this, [=]() { 	
		if (!StartEngineThread())
		{
			DestroyEngineThread();
			return;
		}

		CToolsWindow::Create<CEditorWindow>();

		close();
		deleteLater(); 
	});
	connect(btnShowStartup, &QCheckBox::checkStateChanged, this, [=](Qt::CheckState state) {
		bShowStartup = (state == Qt::Checked);
	});

	RestoreState();

	UpdateProjectList();
}

void CProjectManagerWnd::UpdateProjectList()
{
	projectList->clear();

	for (int i = 0; i < projects.Size(); i++)
	{
		QListWidgetItem* item = new QListWidgetItem(QString(projects[i].name.c_str()), projectList);
		item->setData(Qt::UserRole + 1, QVariant(i));

		if (projects[i].bHasIcon)
		{
			QString iconPath = QString((const QChar*)(projects[i].path + "\\.project\\icon.png").c_str());

			item->setIcon(QPixmap(iconPath));
		}
		else
			item->setIcon(QIcon(":/icons/folder.svg"));
	}
}

void CProjectManagerWnd::CreateNewProject()
{
	CFramelessDialog* dialog = new CFramelessDialog(this);
	QFrame* frame = new QFrame(dialog);
	QVBoxLayout* layout = new QVBoxLayout(frame);
	layout->setSpacing(16);

	//{
	//	QVBoxLayout* l = new QVBoxLayout(dialog);
	//	l->setContentsMargins(0, 0, 0, 0);
	//	l->addWidget(frame);
	//}

	dialog->setCentralWidget(frame);

	QLabel* lbl = new QLabel("Create Project", this);
	QFont font = lbl->font();
	font.setPointSize(20);
	font.setBold(true);
	font.setLetterSpacing(QFont::AbsoluteSpacing, 2.f);
	lbl->setFont(font);
	layout->addWidget(lbl);

	QHBoxLayout* l1 = new QHBoxLayout();
	l1->addWidget(new QLabel("Name: ", dialog));
	l1->setContentsMargins(0, 0, 0, 0);

	QLineEdit* editName = new QLineEdit(dialog);
	editName->setPlaceholderText("Project Name...");
	l1->addWidget(editName);
	layout->addLayout(l1);

	QHBoxLayout* l2 = new QHBoxLayout();
	l2->addWidget(new QLabel("Location: ", dialog));
	l2->setContentsMargins(0, 0, 0, 0);

	QLineEdit* editDir = new QLineEdit(dialog);
	editDir->setText(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Thorium Projects");
	l2->addWidget(editDir);

	QPushButton* browseDir = new QPushButton("Browse", dialog);
	l2->addWidget(browseDir);

	layout->addLayout(l2);

	QPushButton* btn = new QPushButton("Finish", dialog);
	btn->setProperty("type", QVariant("primary"));
	btn->setEnabled(false);
	layout->addWidget(btn);

	connect(browseDir, &QPushButton::clicked, this, [=]() {
		QString dir = QFileDialog::getExistingDirectory(dialog, "Select Folder", editDir->text());
		if (!dir.isEmpty())
			editDir->setText(dir);
		});
	connect(btn, &QPushButton::clicked, this, [=]() { dialog->done(true); });
	connect(editName, &QLineEdit::textChanged, this, [=](const QString& t) { btn->setEnabled(!t.isEmpty()); });

	int x = QGuiApplication::primaryScreen()->geometry().width();
	int y = QGuiApplication::primaryScreen()->geometry().height();

	x -= 400;
	y -= 227;
	x /= 2;
	y /= 2;
	dialog->setGeometry(QRect(x, y, 400, 227));

	int r = dialog->exec();
	dialog->deleteLater();

	if (!r)
		return;

	FString projName = editName->text().toStdString();
	FString projDir = editDir->text().toStdString();

	//CEditorEngine::CreateProject(projName, projDir);

	/*if (!gEngine->LoadProject(projDir + "/" + projName))
		return;*/

	if (!StartEngineThread(projDir + "/" + projName))
	{
		DestroyEngineThread();
		return;
	}

	CToolsWindow::Create<CEditorWindow>();

	close();
	deleteLater();
}

void CProjectManagerWnd::OpenProject()
{
	QString p = QFileDialog::getOpenFileName(this, "Open Project", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Thorium Projects", "Project File (*.thproj)");
	if (p.isEmpty())
		return;

	FString path = p.toStdString();
	path.Erase(path.begin() + path.FindLastOf("/\\"), path.end());
	path += "\\..";

	if (!StartEngineThread(path))
	{
		DestroyEngineThread();
		return;
	}

	//gEditorEngine()->RegisterProject(gEngine->GetProjectConfig());
	CToolsWindow::Create<CEditorWindow>();

	close();
	deleteLater();
}

void CProjectManagerWnd::closeEvent(QCloseEvent* event)
{
	FKeyValue kv(SSystem::GetDataPath() + "/ThoriumEngine/EditorConfig/Editor.cfg");
	kv.SetValue("show_projectbrowser_startup", bShowStartup ? "1" : "0");
	kv.Save();

	event->accept();
}

void CProjectManagerWnd::SearchForProjects()
{
	//QString doc = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

	//if (!FFileHelper::DirectoryExists(doc.toStdString() + "\\Thorium Projects"))
	//	return;

	//for (auto entry : std::filesystem::directory_iterator(doc.toStdString() + "\\Thorium Projects"))
	//{
	//	if (entry.is_regular_file())
	//		continue;

	//	FKeyValue kv(WString(entry.path().wstring()) + L"\\config\\project.cfg");
	//	if (kv.IsOpen())
	//	{
	//		FProjectDef proj;
	//		proj.name = *kv.GetValue("displayName");
	//		proj.path = entry.path().wstring();
	//		proj.bHasIcon = FFileHelper::FileExists(WString(entry.path().wstring()) + L"\\.project\\icon.png");
	//		projects.Add(proj);
	//	}
	//}

	FKeyValue kv(SSystem::GetDataPath() + "/ThoriumEngine/EditorConfig/Editor.cfg");
	if (kv.IsOpen())
	{
		bShowStartup = kv.GetValue("show_projectbrowser_startup")->AsBool(true);

		auto* projects = kv.GetCategory("projects");
		if (!projects)
			return;

		for (auto& proj : projects->GetValues())
		{
			FKeyValue kv(proj.Value.Value + "/config/project.cfg");
			if (kv.IsOpen())
			{
				FProjectDef p;
				p.name = *kv.GetValue("displayName");
				p.path = proj.Value;
				p.bHasIcon = FFileHelper::FileExists(proj.Value.Value + "/.project/icon.png");
				this->projects.Add(p);
			}
		}
	}

	/*for (auto& proj : gEditorEngine()->availableProjects)
	{
		FKeyValue kv(proj.dir + L"\\config\\project.cfg");
		if (kv.IsOpen())
		{
			FProjectDef p;
			p.name = *kv.GetValue("displayName");
			p.path = proj.dir;
			p.bHasIcon = FFileHelper::FileExists(proj.dir + L"\\.project\\icon.png");
			projects.Add(p);
		}
	}*/
}
