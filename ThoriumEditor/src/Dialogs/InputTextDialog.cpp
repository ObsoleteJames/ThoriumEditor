
#define IMGUI_DEFINE_MATH_OPERATORS
#include "InputTextDialog.h"

#include "ImGui/imgui_thorium.h"

CInputTextDialog::CInputTextDialog(const FString& t, const FString& l) : CDialogWnd(t, EDialogFlags_NoResize), label(l)
{
	windowSize = { 300, 90 };
}

void CInputTextDialog::Render()
{
	if (ImGui::Begin("ChoiceDialog", 0, DIALOG_WND_FLAGS))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			Finish(0);

		ImVec2 size = ImGui::GetContentRegionAvail();

		ImVec2 cursor = ImGui::GetCursorPos();
		
		ImGui::InputText((label + "##input").c_str(), &input);
		//ImGui::TextWrapped(msg.c_str());

		auto& style = ImGui::GetStyle();
		ImGui::SetCursorPosY(size.y - 18);
		ImGui::SetCursorPosX(size.x - 128 - style.FramePadding.x);
		if (ImGui::Button("Ok", ImVec2(64, 24)))
			Finish(1);
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(64, 24)))
			Finish();
	}
	ImGui::End();
}
