#pragma once

#include "Layer.h"
#include "EditorCore.h"

struct FProject;

class CProjectManager : public CLayer
{
	static bool bIsOpen;

public:
	void OnUIRender() override;

	void CreateProject(const FString& name, const FString& path);
	void OpenProject(int index);

	void AddProject();

	static void Open(int mode);

	inline static bool IsOpen() { return bIsOpen; }

private:
	void RenderProjectItem(const FProject& proj, int index);

private:
	int selectedProject = -1;

	int mode = 0;
};
