
#define IMGUI_DEFINE_MATH_OPERATORS
#include "ProjectManager.h"
#include "EditorEngine.h"

#include "Misc/FileHelper.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Imgui/imgui_thorium.h"

#define PROJECT_MODE_OPEN 0
#define PROJECT_MODE_CREATE 1

//REGISTER_EDITOR_LAYER(CProjectManager, nullptr, nullptr, false, false)

bool CProjectManager::bIsOpen = false;

CProjectManager::CProjectManager() : CDialogWnd("Project Manager")
{
	windowSize = { 670, 440 };
}

void CProjectManager::Render()
{
	if (ImGui::Begin("Projects##projectManager", 0, DIALOG_WND_FLAGS))
	{
		if (mode == PROJECT_MODE_OPEN)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] * ImVec4(1, 1, 1, 0.8f));
			ImGui::Button("New Project");
			ImGui::PopStyleColor();

			ImGui::SameLine();

			if (ImGui::Button("Add Project"))
				AddProject();

			ImGui::SameLine();

			ImGui::BeginDisabled(selectedProject == -1);
			if (ImGui::Button("Open Project"))
				OpenProject(selectedProject);
			ImGui::EndDisabled();

			if (ImGui::BeginChild("projectlist", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding))
			{
				ImVec2 itemSize = ImVec2(48 + (48 * 2) / 2, 80 + (80 * 2) / 2);
				ImVec2 contentSize = ImGui::GetContentRegionAvail();
				int columns = FMath::Max((int)(contentSize.x / (itemSize.x + 10.f)), 1);

				if (ImGui::BeginTable("projects_table", columns))
				{
					int i = 0;
					for (auto& proj : gEditorEngine()->availableProjects)
					{
						RenderProjectItem(proj, i);

						i++;
					}

					ImGui::EndTable();
				}

				ImGui::EndChild();
			}
		}
		else if (mode == PROJECT_MODE_CREATE)
		{

		}

	}
	ImGui::End();
}

void CProjectManager::RenderProjectItem(const FProject& proj, int index)
{
	bool bSelected = selectedProject == index;

	ImGui::TableNextColumn();
	ImVec2 cursor = ImGui::GetCursorScreenPos();

	if (bSelected)
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);

	ImVec2 itemSize = ImVec2(48 + (48 * 2) / 2, 80 + (80 * 2) / 2);

	if (ImGui::ButtonEx((proj.displayName + "##_" + proj.name).c_str(), itemSize))
		selectedProject = index;
	
	if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
		OpenProject(index);

	if (bSelected)
		ImGui::PopStyleColor();

	uint32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(0.06f, 0.06f, 0.06f, 1.00f));
	ImGui::RenderFrame(cursor, cursor + ImVec2(itemSize.x, itemSize.x), col, false, ImGui::GetStyle().FrameRounding);
	ImGui::RenderFrame(cursor + ImVec2(0, ImGui::GetStyle().FrameRounding + 2.f), cursor + ImVec2(itemSize.x, itemSize.x), col, false);

	ImGui::RenderTextWrapped(cursor + ImVec2(5, itemSize.x + 5), proj.displayName.c_str(), nullptr, itemSize.x - 5);
}

void CProjectManager::CreateProject(const FString& name, const FString& path)
{
}

void CProjectManager::OpenProject(int i)
{
	gEditorEngine()->LoadProject(gEditorEngine()->availableProjects[i].dir);
	Finish();
}

void CProjectManager::AddProject()
{
	if (FString folder = CEngine::OpenFolderDialog(); !folder.IsEmpty())
	{
		FKeyValue kv(folder + "/config/project.cfg");
		if (!kv.IsOpen())
			return;

		FProject proj;
		proj.dir = folder;
		proj.name = *kv.GetValue("name");
		proj.displayName = *kv.GetValue("displayName");

		gEditorEngine()->RegisterProject(proj);
	}
}

void CProjectManager::Open(int mode)
{
	//auto* m = (CProjectManager*)CProjectManagerType_Instance.Instantiate();
	//m->mode = mode;
	//m->bEnabled = true;
	//bIsOpen = true;
	//ImGui::OpenPopup("Projects##projectManager");

	CProjectManager wnd = CProjectManager();
	wnd.mode = mode;
	wnd.Exec();
}
