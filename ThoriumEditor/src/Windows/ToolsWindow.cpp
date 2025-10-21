
#include <string>
#include "Engine.h"
#include "ToolsWindow.h"
#include "EditorEngine.h"
#include "Registry/FileSystem.h"
#include <Util/KeyValue.h>
#include <Util/Assert.h>

#include <fstream>
#include <QMenuBar>
#include <QMouseEvent>
#include <QLabel>
#include <QBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QXmlStreamWriter>

#include "DockManager.h"

//TArray<FToolsWindowClass*> _RegisteredWindows;
//TArray<FToolsWidgetClass*> _RegisteredWidgets;

TMap<SizeType, CToolsWindow*> _Windows;
TMap<SizeType, CToolsWidget*> _Widgets;

FString _StyleSheet;

CToolsWindow::CToolsWindow(QWidget* parent) : QMainWindow(parent)
{
	_Windows[ID] = this;

	menuBar = new QMenuBar(this);
	menuBar->setGeometry(0, 0, 0, 26);
	menuBar->setObjectName("menuBar");
	menuBar->setLayoutDirection(Qt::LeftToRight);
	setMenuBar(menuBar);

	//setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);
}

CToolsWindow::~CToolsWindow()
{
	auto it = _Windows.find(ID);
	if (it != _Windows.end())
		_Windows.erase(it);
}

FToolsWindowClass* CToolsWindow::GetClassById(SizeType _id)
{
	for (auto& wnd : ToolsRegisteredWindows::Get())
		if (wnd->Id == _id)
			return wnd;

	return nullptr;
}

CToolsWindow* CToolsWindow::CreateById(SizeType _id)
{
	for (const auto& wnd : ToolsRegisteredWindows::Get())
	{
		if (wnd->Id == _id)
			return wnd->Create();
	}
	return nullptr;
}

CToolsWindow* CToolsWindow::GetByName(const char* name)
{
	for (auto it : _Windows)
	{
		if (strcmp(it.second->Name, name) == 0)
			return it.second;
	}
	return nullptr;
}

bool CToolsWindow::CloseAll(CToolsWindow* ignore)
{
	if (_Windows.size() == 0)
		return true;

	for (auto it = _Windows.rbegin(); it != _Windows.rend(); it++)
	{
		if (it->second == ignore)
			continue;

		if (!it->second->Shutdown())
			return false;
		else
			it->second->destroy();
	}

	return true;
}

//void CToolsWindow::ReloadStyle()
//{
//	_StyleSheet.Clear();
//	for (auto& wnd : _Windows)
//		wnd.second->setStyleSheet(GetStyleSheet().c_str());
//}
//
//FString& CToolsWindow::GetStyleSheet()
//{
//	if (_StyleSheet.IsEmpty())
//	{
//		FString stylePath = "editor\\themes\\" + gEditorEngine()->config.theme + "\\theme";
//
//		FFile* file = CFileSystem::FindFile(ToWString(stylePath));
//		THORIUM_ASSERT(file, "Failed to find theme" + gEditorEngine()->config.theme);
//
//		FString enginePath = ToFString(file->Mod()->Path()) + "\\editor\\themes\\" + gEditorEngine()->config.theme;
//
//		FKeyValue theme(file->FullPath());
//		THORIUM_ASSERT(theme.IsOpen(), "Failed to open theme file '" + ToFString(file->FullPath()) + "'");
//
//		for (auto& v : *theme.GetArray("include", true))
//		{
//			std::ifstream includeStream((enginePath + "\\" + v).c_str());
//			THORIUM_ASSERT(includeStream.is_open(), FString("Failed to open '") + v + "'");
//			std::string _l;
//			while (std::getline(includeStream, _l))
//				_StyleSheet += _l + '\n';
//		}
//
//		//std::ifstream stream(file->FullPath().c_str());
//		//THORIUM_ASSERT(stream.is_open(), "Failed to open theme file '" + ToFString(file->FullPath()) + "'");
//
//		//std::string line;
//		//while (std::getline(stream, line))
//		//{
//		//	size_t inclI = line.find('#');
//		//	if (inclI != -1)
//		//	{
//		//		FString inclFile;
//		//		const char* ptr = line.c_str() + inclI + 8;
//		//		bool bInQuotes = false;
//		//		while (ptr[0] != '\0')
//		//		{
//		//			char ch = ptr[0];
//		//			if (ch == '\n')
//		//				break;
//
//		//			if (ch == '"')
//		//			{
//		//				bInQuotes ^= 1;
//		//				ptr++;
//		//				continue;
//		//			}
//
//		//			if (ch == ' ' && !bInQuotes)
//		//			{
//		//				ptr++;
//		//				continue;
//		//			}
//
//		//			inclFile += ch;
//		//			ptr++;
//		//		}
//
//		//		std::ifstream includeStream((enginePath + "\\bin\\styles\\" + inclFile).c_str());
//		//		THORIUM_ASSERT(includeStream.is_open(), FString("Failed to open '") + inclFile + "'");
//		//		std::string _l;
//		//		while (std::getline(includeStream, _l))
//		//		{
//		//			_StyleSheet += _l + '\n';
//		//		}
//		//	}
//		//}
//	}
//
//	return _StyleSheet;
//}

bool CToolsWindow::CloseWindow()
{
	if (!Shutdown())
		return false;

	deleteLater();
	return true;
}

bool CToolsWindow::Shutdown()
{
	return true;
}

void CToolsWindow::SetupUi()
{
}

void CToolsWindow::SaveState()
{
	QString appdataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "\\ThoriumEngine";
	QSettings settings(appdataPath + "\\EditorConfig\\" + Name + ".cfg", QSettings::Format::IniFormat);

	settings.setValue("window_state", saveState());
	settings.setValue("window_geo", saveGeometry());

	QList<QDockWidget*> dockWidgets = findChildren<QDockWidget*>();
	//QList<ads::CDockWidget*> dockWidgets = findChildren<ads::CDockWidget*>();

	for (auto* dock : dockWidgets)
	{
		settings.beginGroup(QString("dock_") + dock->objectName());

		settings.setValue("dockgeo", dock->saveGeometry());

		settings.endGroup();
	}

	if (dockmanager)
		settings.setValue("adsDocks", dockmanager->saveState());

	UserSaveState(settings);
}

void CToolsWindow::RestoreState()
{
	QString appdataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "\\ThoriumEngine";
	QSettings settings(appdataPath + "\\EditorConfig\\" + Name + ".cfg", QSettings::Format::IniFormat);

	restoreState(settings.value("window_state").toByteArray());
	restoreGeometry(settings.value("window_geo").toByteArray());

	QList<QDockWidget*> dockWidgets = findChildren<QDockWidget*>();

	for (auto* dock : dockWidgets)
	{
		settings.beginGroup(QString("dock_") + dock->objectName());

		dock->restoreGeometry(settings.value("dockgeo").toByteArray());

		settings.endGroup();
	}

	if (dockmanager)
		dockmanager->restoreState(settings.value("adsDocks").toByteArray());

	UserRestoreState(settings);
}

TMap<SizeType, CToolsWindow*>& CToolsWindow::GetAll()
{
	return _Windows;
}
