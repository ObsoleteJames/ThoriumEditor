
#include "ConsoleWidget.h"

#include "Console.h"
#include "EditorEngine.h"
#include "EditorMenu.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

REGISTER_EDITOR_LAYER(CConsoleWidget, "View/Console", nullptr, false, true)

static FString TimeToHmsString(time_t* time)
{
	struct tm time_info;
	char timeString[9];  // space for "HH:MM:SS\0"

	localtime_s(&time_info, time);

	strftime(timeString, sizeof(timeString), "%H:%M:%S", &time_info);
	timeString[8] = '\0';
	return FString(timeString);
}

void CConsoleWidget::OnUIRender()
{
	if (ImGui::Begin("Console##_editorConsoleWidget", &bEnabled))
	{
		static char buffInput[48] = { '\0' };
		static char buffFilter[64] = { '\0' };

		if (ImGui::BeginChild("consloleScrollArea", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_AlwaysUseWindowPadding))
		{
			for (auto& msg : CConsole::GetMsgCache())
			{
				if (StrLen(buffFilter) > 0)
				{
					if (msg.msg.Find(buffFilter) == -1)
						continue;
				}

				if ((msg.type == CONSOLE_PLAIN || msg.type == CONSOLE_INFO) && !bShowInfo)
					continue;
				if (msg.type == CONSOLE_WARNING && !bShowWarnings)
					continue;
				if (msg.type == CONSOLE_ERROR && !bShowErrors)
					continue;

				auto ts = ImGui::CalcTextSize(msg.msg.c_str());

				ImGui::Selectable(("##_msg" + FString::ToString((SizeType)&msg)).c_str(), false, ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_SpanAvailWidth, ImVec2(0, ts.y));
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
					ImGui::SetTooltip(("Function: " + FString(msg.info.function) + "\nFile: " + msg.info.file + "\nLine: " + FString::ToString(msg.info.line)).c_str());

				ImGui::SameLine();

				FString msgTime = TimeToHmsString((time_t*)&msg.time);

				if (msg.type != CONSOLE_PLAIN)
					ImGui::TextColored(ImVec4(0.435f, 0.7f, 0.294f, 1.f), ("[" + msgTime + "] " + msg.module).c_str());

				ImGui::SameLine();

				if (msg.type == CONSOLE_PLAIN)
					ImGui::TextColored(ImVec4(0.58f, 0.58f, 0.58f, 1.f), msg.msg.c_str());
				else if (msg.type == CONSOLE_INFO)
					ImGui::TextColored(ImVec4(0.78f, 0.78f, 0.78f, 1.f), msg.msg.c_str());
				else if (msg.type == CONSOLE_WARNING)
					ImGui::TextColored(ImVec4(0.9f, 0.77f, 0.26f, 1.f), msg.msg.c_str());
				else if (msg.type == CONSOLE_ERROR)
					ImGui::TextColored(ImVec4(0.811f, 0.125f, 0.09f, 1.f), msg.msg.c_str());
			}

			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);
		}
		ImGui::EndChild();

		float contentWidth = ImGui::GetContentRegionAvail().x;

		ImGui::InputText("##_consoleInput", buffInput, 48);
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			FString input = buffInput;
			if (!input.IsEmpty())
				CConsole::Exec(input);

			buffInput[0] = '\0';
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(contentWidth * 0.2f);

		ImVec2 curPos = ImGui::GetCurrentWindow()->DC.CursorPos;
		ImGui::InputText("##_filter", buffFilter, 64, 0);

		if (StrLen(buffFilter) == 0)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.4f));
			ImGui::RenderText(curPos + ImGui::GetCurrentContext()->Style.FramePadding, "Filter...");
			ImGui::PopStyleColor();
		}

		ImGui::SameLine();
		bool bPop = false;
		if (!bShowInfo)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.22f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.4f));
			bPop = true;
		}

		if (ImGui::Button("Info"))
			bShowInfo = ! bShowInfo;

		if (bPop)
		{
			ImGui::PopStyleColor(2);
			bPop = false;
		}

		ImGui::SameLine();
		if (!bShowWarnings)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.22f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.4f));
			bPop = true;
		}

		if (ImGui::Button("Warnings"))
			bShowWarnings ^= 1;

		if (bPop)
		{
			ImGui::PopStyleColor(2);
			bPop = false;
		}

		ImGui::SameLine();
		if (!bShowErrors)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.22f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.4f));
			bPop = true;
		}

		if (ImGui::Button("Errors"))
			bShowErrors ^= 1;

		if (bPop)
		{
			ImGui::PopStyleColor(2);
			bPop = false;
		}

	}
	ImGui::End();
	Menu()->bChecked = bEnabled;
}
