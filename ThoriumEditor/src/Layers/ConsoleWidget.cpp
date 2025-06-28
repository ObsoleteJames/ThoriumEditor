
#include "ConsoleWidget.h"

#include "Console.h"
#include "EditorEngine.h"
#include "EditorMenu.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "ThemeManager.h"

#include "Platform/Windows/DirectX/DirectXTexture.h"

#define TEX_VIEW(tex) ((DirectXTexture2D*)tex)->view

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

		if (ImGui::BeginChild("consloleScrollArea", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_AlwaysUseWindowPadding))
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

				if (msg.repeats > 1)
				{
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(1, 1, 1, 0.4f), "x%d", msg.repeats);
				}

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

		float contentWidth = ImGui::GetContentRegionAvail().x - 110;

		ImGui::SetNextItemWidth(contentWidth * 0.8f);

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

		ITexture2D* imgInfo = ThoriumEditor::GetThemeIcon("consoleInfo24");
		ITexture2D* imgWarning = ThoriumEditor::GetThemeIcon("consoleWarning24");
		ITexture2D* imgError = ThoriumEditor::GetThemeIcon("consoleError24");

		ImGui::SameLine();

		if (ImGui::ImageButtonClear("##btnShowInfo", TEX_VIEW(imgInfo), ImVec2(24, 24), 0, ImVec2(0, 0), ImVec2(1, 1), bShowInfo ? ImVec4(1, 1, 1, 1) : ImVec4(1, 1, 1, 0.3f)))
			bShowInfo = !bShowInfo;

		ImGui::SetItemTooltip("Show Info");

		//if (ImGui::Button("Info"))

		ImGui::SameLine();

		if (ImGui::ImageButtonClear("##btnShowWarnings", TEX_VIEW(imgWarning), ImVec2(24, 24), 0, ImVec2(0, 0), ImVec2(1, 1), bShowWarnings ? ImVec4(1, 1, 1, 1) : ImVec4(1, 1, 1, 0.3f)))
			bShowWarnings ^= 1;

		ImGui::SetItemTooltip("Show Warnings");

		ImGui::SameLine();

		if (ImGui::ImageButtonClear("##btnShowErrors", TEX_VIEW(imgError), ImVec2(24, 24), 0, ImVec2(0, 0), ImVec2(1, 1), bShowErrors ? ImVec4(1, 1, 1, 1) : ImVec4(1, 1, 1, 0.3f)))
			bShowErrors ^= 1;

		ImGui::SetItemTooltip("Show Errors");

	}
	ImGui::End();
	Menu()->bChecked = bEnabled;
}
