
#include "EngineThread.h"
#include "EditorEngine.h"

#include <QApplication>
#include "Misc/Timer.h"

#include <chrono>
#include <thread>

CEngineThread::CEngineThread(QObject* parent /*= nullptr*/) : QThread(parent)
{
}

void CEngineThread::run()
{
	gIsRunning = true;

	FTimer dtTimer;
	while (gIsRunning)
	{
		if (bPaused)
		{
			msleep(8);
			continue;
		}
		dtTimer.Begin();

		emit onUpdate();
		gEngine->Run();

		dtTimer.Stop();
		gEditorEngine->SetDeltaTime(dtTimer.GetSeconds());
	}

	gEditorEngine->OnExit();
}

void CEngineThread::Pause()
{
	bPaused = true;
}

void CEngineThread::Resume()
{
	bPaused = false;
}
