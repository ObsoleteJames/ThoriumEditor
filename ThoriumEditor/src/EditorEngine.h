#pragma once

#include "Editor.h"
#include "Engine.h"

#include <mutex>

class CEditorWindow;
class IFrameBuffer;

enum EEventExec
{
	EventExec_PreUpdate,
	EventExec_PostUpdate,
	EventExec_PreRender,
	EventExec_PostRender
};

// events used to communicate between threads.
class IEditorEvent
{
public:
	virtual void Exec() = 0;

public:
	EEventExec execTime = EventExec_PreUpdate;
};

class EDITOR_API CEditorEngine : public CEngine
{
	friend class CEngineThread;

public:
	CEditorEngine() = default;

	void Init() override;

	int Run() override;

	void OnExit() override;

public:
	void PushEvent(IEditorEvent* event);

	void SelectObject(CObject* obj);
	void SelectObjects(const TArray<CObject*>& objs);
	void AddSelectedObject(CObject* obj);
	void RemoveSelectedObject(CObject* obj);
	bool IsObjectSelected(CObject* obj);
	void ClearSelection();

private:
	void OnLevelChange();

	void UpdateEvents(EEventExec time);

	void SetDeltaTime(double dt);

	void DoEditorRender();

public: // Editor Variables
	bool bIsPlaying = false;
	bool bPaused = false;
	bool bStepFrame = false;

	std::mutex eventMutex;
	TArray<IEditorEvent*> events;

	TObjectPtr<CObject> activeObject;
	TArray<TObjectPtr<CObject>> selectedObjects;

public: // Rendering
	bool bSelectionBoundingBox = true;
	bool bSelectionOverlay = true;
	bool bSelectionSizeText = true;
	bool bGameView = false;

	CCameraProxy* viewportCams[4];

};

inline CEditorEngine* gEditorEngine() { return (CEditorEngine*)gEngine; }
