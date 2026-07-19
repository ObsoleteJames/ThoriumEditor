#pragma once

#include "Editor.h"
#include <QThread>

class EDITOR_API CEngineThread : public QThread
{
	Q_OBJECT

public:
	CEngineThread(QObject* parent = nullptr);

	void run() override;

	void Pause();
	void Resume();

signals:
	void onUpdate();

	void onSelectionChanged();
	void onLevelChanged();
	void onThumbnailGenerated();

private:
	bool bPaused = false;
};
