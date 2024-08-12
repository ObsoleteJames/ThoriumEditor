
#define IMGUI_DEFINE_MATH_OPERATORS
#include "ChoiceDialog.h"

#include "ImGui/imgui_thorium.h"

CChoiceDialog::CChoiceDialog(const FString& t, const FString& m, EOption o) : CDialogWnd(t, EDialogFlags_NoResize), msg(m), option(o)
{
	windowSize = { 300, 90 };
}

void CChoiceDialog::Render()
{
	if (ImGui::Begin("ChoiceDialog", 0, DIALOG_WND_FLAGS))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			Finish(0);

		ImVec2 size = ImGui::GetContentRegionAvail();

		ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::SetCursorPos(cursor + ImVec2(2, 1));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
		ImGui::TextWrapped(msg.c_str());
		ImGui::PopStyleColor();
		ImGui::SetCursorPos(cursor);
		ImGui::TextWrapped(msg.c_str());

		auto& style = ImGui::GetStyle();

		ImGui::SetCursorPosY(size.y - 18);
		switch (option)
		{
		case OPTION_OK:
			ImGui::SetCursorPosX(size.x - 64);
			if (ImGui::Button("Ok", ImVec2(64, 24)))
				Finish();
			break;
		case OPTION_OK_CANCEL:
			ImGui::SetCursorPosX(size.x - 128 - style.FramePadding.x);
			if (ImGui::Button("Ok", ImVec2(64, 24)))
				Finish(1);
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(64, 24)))
				Finish();
			break;
		case OPTION_YES_NO:
			ImGui::SetCursorPosX(size.x - 128 - style.FramePadding.x);
			if (ImGui::Button("Yes", ImVec2(64, 24)))
				Finish(1);
			ImGui::SameLine();
			if (ImGui::Button("No", ImVec2(64, 24)))
				Finish();
			break;
		case OPTION_YES_NO_CANCEL:
			ImGui::SetCursorPosX(size.x - 192 - style.FramePadding.x * 2);
			if (ImGui::Button("Yes", ImVec2(64, 24)))
				Finish(2);
			ImGui::SameLine();
			if (ImGui::Button("No", ImVec2(64, 24)))
				Finish(1);
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(64, 24)))
				Finish();
			break;
		}
	}
	ImGui::End();
}
