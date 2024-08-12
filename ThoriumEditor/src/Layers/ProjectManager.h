#pragma once

#include "Layer.h"
#include "Dialogs/Dialog.h"
#include "EditorCore.h"

struct FProject;

class CProjectManager : public CDialogWnd
{
	static bool bIsOpen;

public:
	CProjectManager();

	void Render() override;

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
