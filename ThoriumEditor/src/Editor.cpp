
#include "Editor.h"
#include "EditorEngine.h"
#include "EngineThread.h"
#include "System.h"

#include <QObject>
#include <QSplashScreen>

CEngineThread* gEngineThread = nullptr;
QSplashScreen* gSplashscreen = nullptr;

void _EngineThread()
{
	//gEngine->LoadProject("C:\\Users\\theon\\Documents\\Thorium Projects\\RollerCoasterGame");
	gEngine->Run();
}

bool StartEngineThread(const FString& project)
{
	bool r = true;

	auto enginePath = SSystem::GetEnginePath();
	gSplashscreen = new QSplashScreen(QPixmap((enginePath + "/content/editor/splash.png").c_str()));
	gSplashscreen->show();
	gSplashscreen->showMessage("Loading Engine...", Qt::AlignLeft | Qt::AlignBottom, Qt::white);
	
	// TODO: scan for libraries that may need compiling!

	gEngine = new CEditorEngine();
	gEngine->Init();

	if (!project.IsEmpty())
		if (!gEngine->LoadProject(project))
			r = false;

	gEngineThread = new CEngineThread();
	//QObject::connect(gEngineThread, &CEngineThread::finished, gEngineThread, &CEngineThread::deleteLater);

	gEngineThread->start();
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
