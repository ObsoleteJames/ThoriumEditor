#pragma once

#include "Editor.h"
#include <Util/Core.h>

#include <QObject>
#include <QIcon>

class QAction;

class EDITOR_API IEditorTool : public QObject
{
	Q_OBJECT

	friend class CEditorWindow;

public:
	virtual void Init() = 0;
	virtual void Shutdown() {}

	virtual void Enable() {}
	virtual void Disable() {}

	// called every frame
	virtual void Update() {}

	inline const QIcon& getIcon() const { return icon; }

signals:
	void onEnabled();
	void onDisabled();

protected:
	QIcon icon;

};
