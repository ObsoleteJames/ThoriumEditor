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
	friend class CEditorWindow;

public:
	CEditorEngine() = default;

	void Init() override;

	int Run() override;

	void OnExit() override;

public:
	void PushEvent(IEditorEvent* event);
	void PushEvent(EEventExec time, std::function<void()> func);

	void SelectObject(CObject* obj);
	void SelectObjects(const TArray<CObject*>& objs);
	void AddSelectedObject(CObject* obj);
	void RemoveSelectedObject(CObject* obj);
	bool IsObjectSelected(CObject* obj);
	void ClearSelection();

	// Returns the selected objects of the specified type.
	template<typename T>
	TArray<T*> GetSelectedObjects();

	void BakeLighting();

private:
	void OnLevelChange();
	void OnSaveScene();

	void UpdateEvents(EEventExec time);

	void SetDeltaTime(double dt);

	void InitEditorRender();
	void DoEditorRender();

public: // Editor Variables
	bool bIsPlaying = false;
	bool bPaused = false;
	bool bStepFrame = false;

	std::mutex eventMutex;
	TArray<IEditorEvent*> events;

	TObjectPtr<CObject> activeObject;
	TArray<TObjectPtr<CObject>> selectedObjects;

public:
	// Grid
	bool bGridSnap = false;
	bool bAngleSnap = false;
	float gridSize = 1.0f;
	float angleSnap = 15.f;
	
	// Rendering
	bool bDrawGrid = true;
	bool bSelectionBoundingBox = true;
	bool bSelectionOverlay = true;
	bool bSelectionSizeText = true;
	bool bDrawGizmos = true;
	bool bGameView = false;

	CCameraProxy* viewportCams[4];

	TObjectPtr<CShaderSource> shaderSelectOverlay;
	TObjectPtr<IGBuffer> objectBuffer;
	TObjectPtr<IGBuffer> sceneBuffer;
};

#define gEditorEngine ((CEditorEngine*)gEngine)

template<typename T>
inline TArray<T*> CEditorEngine::GetSelectedObjects()
{
	TArray<T*> r;
	for (auto& obj : selectedObjects)
		if (T* c = Cast<T>(obj); c != nullptr)
			r.Add(c);

	return r;
}
