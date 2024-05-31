
#include "EditorLog.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "EditorWidgets.h"
#include "EditorEngine.h"
#include "EditorMenu.h"

REGISTER_EDITOR_LAYER(CEditorLogWnd, "View/Log", nullptr, false, false)

inline TArray<FEditorLog*>& Logs()
{
    static TArray<FEditorLog*> logs;
    return logs;
}

FEditorLog::FEditorLog(const FString& n) : name(n)
{
    Logs().Add(this);
}

FEditorLog::~FEditorLog()
{
    Logs().Erase(Logs().Find(this));
}

void CEditorLogWnd::OnUIRender()
{
    if (ImGui::Begin("Log##editorLogWnd", &bEnabled))
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
	    float sizeR = size.x - sizeL;
        ImGui::Splitter("##editorLogSplit", false, 4, &sizeL, &sizeR);

        if (ImGui::BeginChild("logs", ImVec2(sizeL, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding))
        {
            for (int i = 0; i < Logs().Size(); i++)
            {
                bool bSelected = i == curIndex;
                if (ImGui::Selectable(Logs()[i]->Name().c_str(), bSelected))
                {
                    curIndex = i;
                }
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        if (ImGui::BeginChild("logtext", ImVec2(sizeR, 0)))
        {
            ImGui::TextWrapped(Logs()[curIndex]->Log().c_str());
        }
        ImGui::EndChild();
    }
	ImGui::End();
	Menu()->bChecked = bEnabled;
}

void CEditorLogWnd::OpenLog(const FString &name)
{
    for (int i = 0; i < Logs().Size(); i++)
    {
        if (Logs()[i]->Name() == name)
        {
            curIndex = i;
            break;
        }
    }
}

void CEditorLogWnd::OpenLog(int index)
{
    if (index > 0 && index < Logs().Size())
        curIndex = index;
}
