
#define IMGUI_DEFINE_MATH_OPERATORS
#include "EditorEngine.h"
#include "FileDialog.h"
#include "AssetBrowserWidget.h"

#include "ImGui/imgui_thorium.h"

CSaveFileDialog::CSaveFileDialog(FClass* type, const FString& dir /*= FString()*/)
{
	windowSize = { 900, 600 };

	browser = new CAssetBrowserWidget();
	browser->bAllowFileEdit = false;

	if (!dir.IsEmpty())
	{
		FString m;
		FString d;
		browser->ExtractPath(dir, m, d);
		browser->SetDir(m, d);
	}
	else
		browser->SetDir(gEditorEngine()->ActiveGame().mod ? gEditorEngine()->ActiveGame().mod->Name() : "Engine", FString());

	browser->viewFilter = (FAssetClass*)type;
	browser->bDoubleClickedFile = false;
}

void CSaveFileDialog::Render()
{
	if (ImGui::Begin("ChoiceDialog", 0, DIALOG_WND_FLAGS))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			Finish(0);

		ImVec2 size = ImGui::GetContentRegionAvail();

		browser->RenderUI(0, size.y - 32);

		ImGui::InputText("Name", &fileName);
		ImGui::SameLine();

		ImGui::SetCursorPosX(size.x - 90);

		if (ImGui::Button("Cancel"))
		{
			Finish(0);
		}
		ImGui::SameLine();

		if (fileName.IsEmpty())
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
		}

		if (ImGui::Button("Save") && !fileName.IsEmpty())
		{
			outPath = browser->dir + "/" + fileName;
			mod = browser->mod;
			Finish(1);
		}

		if (fileName.IsEmpty())
			ImGui::PopStyleColor(3);
	}
	ImGui::End();
}

