
#include "EngineThread.h"
#include "EditorEngine.h"

#include <QApplication>
#include "Misc/Timer.h"

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

		//FTimer timer;
		//timer.Begin();

		emit onUpdate();
		gEngine->Run();

		//timer.Stop();

		//double targetMs = 2.0;
		//double sleepTime = FMath::Max(targetMs - timer.GetMiliseconds(), 0.0);

		//usleep(ulong(sleepTime * 10));

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
