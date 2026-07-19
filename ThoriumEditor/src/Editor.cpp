
#include "Editor.h"
#include "EditorEngine.h"
#include "EngineThread.h"
#include "System.h"
#include <Util/KeyValue.h>

#include <QObject>
#include <QSplashScreen>
#include <QMessageBox>
#include <QApplication>
#include <filesystem>
#include <Misc/FileHelper.h>

CEngineThread* gEngineThread = nullptr;
QSplashScreen* gSplashscreen = nullptr;

#ifdef CONFIG_RELEASE
#define CMAKE_CONFIG_NAME "Release"
#elif CONFIG_DEVELOPMENT
#define CMAKE_CONFIG_NAME "RelWithDebInfo"
#else
#define CMAKE_CONFIG_NAME "Debug"
#endif

void ScanAddon(const FString& p, TArray<FAddon>& out, const std::filesystem::file_time_type& engineBinTime)
{
	FAddon addon;
	FKeyValue cfg(p + "/addon.cfg");
	if (!cfg.IsOpen())
		return;

	if (cfg.GetValue("hasCode")->AsBool())
	{
		addon.identity = *cfg.GetValue("identity");
		addon.name = *cfg.GetValue("name");
		addon.path = p;
		FString dll = p + "/bin/" PLATFORM_NAME "/" CONFIG_NAME "/" + addon.identity + ".dll";
		if (FFileHelper::FileExists(dll))
		{
			if (std::filesystem::last_write_time(dll.c_str()) > engineBinTime)
				return;
		}

		out.Add(addon);
	}
}

void ScanAddonsForCompilation(const FString& project)
{
	FString enginePath = SSystem::GetEnginePath();

	gSplashscreen->showMessage("Scanning Addons...", Qt::AlignLeft | Qt::AlignBottom, Qt::white);

	TArray<FAddon> addons;

	auto engineBinTime = std::filesystem::last_write_time((enginePath + "/bin/" PLATFORM_NAME "/Engine.dll").c_str());

	for (auto entry : std::filesystem::directory_iterator((enginePath + "/content/addons").c_str()))
	{
		if (!entry.is_directory())
			continue;

		FString p = enginePath + "/content/addons/" + entry.path().filename().generic_string().c_str();
		ScanAddon(p, addons, engineBinTime);
	}

	if (!project.IsEmpty())
	{
		FString projectAddonPath = project + "/addons";
		try 
		{
			for (auto entry : std::filesystem::directory_iterator(projectAddonPath.c_str()))
			{
				if (!entry.is_directory())
					continue;

				FString p = projectAddonPath + "/" + entry.path().filename().generic_string().c_str();
				ScanAddon(p, addons, engineBinTime);
			}
		}
		catch (std::exception& e)
		{
			CONSOLE_LogError("ThoriumEditor", e.what());
		}
	}

	if (addons.Size() > 0)
	{
		auto r = QMessageBox::information(nullptr, "Addon Compilation Required", QString("%1 addon(s) are out of date or have not been compiled! would you like to compile the addon(s)?\nignoring this could cause unexpected errors!").arg(addons.Size()), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
		if (r == QMessageBox::Yes)
		{
			bool bAllCompiled = true;

			for (auto& addon : addons)
			{
				gSplashscreen->showMessage(QString("Compiling Addon '%1'...").arg(addon.name.c_str()), Qt::AlignLeft | Qt::AlignBottom, Qt::white);
				
				FString cmd = SSystem::GetEnginePath(ENGINE_VERSION) + "/bin/win64/BuildTool.exe \"";
				//cmd += CFileSystem::GetCurrentPath() + "/.project/" + activeGame.name + "/Build.cfg\" ";
				cmd += addon.path + "/Build.cfg\" ";
#if PLATFORM_WINDOWS
				cmd += "-x64 ";
#endif
				SSystem::Execute(cmd);
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(500ms);
				if (SSystem::Execute("cmake -A x64 -B \"" + addon.path + "/Intermediate/Build\" \"" + addon.path + "/Intermediate\""))
				{
					bAllCompiled = false;
					CONSOLE_LogError("Editor", "Failed to generate build files for addon: '" + addon.name + "'");
					break;
				}
				std::this_thread::sleep_for(500ms);
				if (SSystem::Execute("cmake -DCMAKE_BUILD_TYPE=" CMAKE_CONFIG_NAME " --build \"" + addon.path + "/Intermediate/Build\""))
				{
					bAllCompiled = false;
					CONSOLE_LogError("Editor", "Failed to compile addon: '" + addon.name + "'");
					break;
				}
				std::this_thread::sleep_for(500ms);
			}
			
			if (!bAllCompiled)
				QMessageBox::warning(nullptr, "Addon Compilation Failed", "One or more addons failed to compile! check the console for more information.");
		}
	}
}

bool StartEngineThread(const FString& project)
{
	bool r = true;

	qApp->setQuitOnLastWindowClosed(false);

	auto enginePath = SSystem::GetEnginePath();
	gSplashscreen = new QSplashScreen(QPixmap((enginePath + "/content/editor/splash.png").c_str()));
	gSplashscreen->show();
	gSplashscreen->showMessage("Initializing...", Qt::AlignLeft | Qt::AlignBottom, Qt::white);
	
	ScanAddonsForCompilation(project);
	gSplashscreen->show();

	gSplashscreen->showMessage("Loading Engine...", Qt::AlignLeft | Qt::AlignBottom, Qt::white);

	gEngine = new CEditorEngine();
	gEngine->Init();

	if (!project.IsEmpty())
		if (!gEngine->LoadProject(project))
			r = false;

	gEngineThread = new CEngineThread();
	//QObject::connect(gEngineThread, &CEngineThread::finished, gEngineThread, &CEngineThread::deleteLater);

	gEngineThread->start();

	qApp->setQuitOnLastWindowClosed(true);
	return r;
}

void StopEngineThread()
{
	gEngine->Exit();
	gEngineThread->wait();
}

void DestroyEngineThread()
{
	if (!gEngineThread)
		return;

	if (!gEngineThread->isFinished())
		StopEngineThread();

	delete gEngine;
	gEngineThread->deleteLater();
}
