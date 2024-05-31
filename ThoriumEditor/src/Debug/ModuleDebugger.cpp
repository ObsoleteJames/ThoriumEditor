
#include "ModuleDebugger.h"
#include "EditorEngine.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Imgui/imgui_thorium.h"

REGISTER_EDITOR_LAYER(CModuleDebugger, "Debug/Module Debugger", nullptr, false, false)

void CModuleDebugger::OnUIRender()
{
	if (ImGui::Begin("Module Debugger", &bEnabled))
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
		if (ImGui::BeginChild("##moduleList", ImVec2(200, 0)))
		{
			auto& modules = CModuleManager::GetModules();

			for (CModule* m : modules)
			{
				if (ImGui::TreeNode(m->Name().c_str()))
				{
					if (ImGui::TreeNodeEx(("Classes##" + m->Name()).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
					{
						for (auto* c : m->Classes)
						{
							if (ImGui::Selectable(c->GetName().c_str(), c == activeStruct))
								SetActive(c);
						}

						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx(("Structs##" + m->Name()).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
					{
						for (auto* c : m->Structures)
						{
							if (ImGui::Selectable(c->GetName().c_str(), c == activeStruct))
								SetActive(c);
						}
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx(("Enums##" + m->Name()).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
					{
						for (auto* c : m->Enums)
						{
							if (ImGui::Selectable(c->GetName().c_str(), c == activeEnum))
								SetActive(c);
						}
						ImGui::TreePop();
					}

					ImGui::TreePop();
				}
			}

		}
		ImGui::PopStyleColor();
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
		if (ImGui::BeginChild("##classViewer"))
		{
			if (ImGui::BeginTable("tableClassViewer", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
			{
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Value");
				ImGui::TableHeadersRow();

				FString name = activeStruct ? activeStruct->name : (activeAsset ? activeAsset->name : (activeEnum ? activeEnum->name : FString()));
				FString iname = activeStruct ? activeStruct->cppName : (activeAsset ? activeAsset->cppName : (activeEnum ? activeEnum->cppName : FString()));
				FString info = activeStruct ? activeStruct->description : (activeAsset ? activeAsset->description : (activeEnum ? activeEnum->description : FString()));
				SizeType size = activeStruct ? activeStruct->Size() : (activeAsset ? activeAsset->Size() : (activeEnum ? activeEnum->Size() : 0));

				ImGui::BeginDisabled(!(activeStruct || activeAsset || activeEnum));

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Name");
				ImGui::TableNextColumn();
				ImGui::InputText("##nameEdit", &name, ImGuiInputTextFlags_ReadOnly);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Internal Name");
				ImGui::TableNextColumn();
				ImGui::InputText("##inameEdit", &iname, ImGuiInputTextFlags_ReadOnly);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Description");
				ImGui::TableNextColumn();
				ImGui::InputText("##infoEdit", &info, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_Multiline);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Size");
				ImGui::TableNextColumn();
				ImGui::DragInt("##sizeEdit", (int*)&size, 1, 0, 0, "%d", ImGuiSliderFlags_ReadOnly);

				if (activeStruct || activeAsset)
				{
					FStruct* s = activeStruct ? activeStruct : activeAsset;
					FClass* c = activeStruct->IsClass() ? (FClass*)activeStruct : nullptr;

					if (ImGui::TableTreeHeader("Properties", 0, true))
					{
						const FProperty* p = s->GetPropertyList();
						while (p)
						{
							if (ImGui::TableTreeHeader(p->name.c_str(), 0, true))
							{
								FString id = FString::ToString((SizeType)p);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Name");
								ImGui::TableNextColumn();
								ImGui::InputText(("##nameEdit" + id).c_str(), (FString*)&p->name, ImGuiInputTextFlags_ReadOnly);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Internal Name");
								ImGui::TableNextColumn();
								ImGui::InputText(("##inameEdit" + id).c_str(), (FString*)&p->cppName, ImGuiInputTextFlags_ReadOnly);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Typename");
								ImGui::TableNextColumn();
								ImGui::InputText(("##typeEdit" + id).c_str(), (FString*)&p->typeName, ImGuiInputTextFlags_ReadOnly);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Size");
								ImGui::TableNextColumn();
								ImGui::DragInt(("##sizeEdit" + id).c_str(), (int*)&p->size, 1, 0, 0, "%d", ImGuiSliderFlags_ReadOnly);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Offset");
								ImGui::TableNextColumn();
								ImGui::DragInt(("##offsetEdit" + id).c_str(), (int*)&p->offset, 1, 0, 0, "%d", ImGuiSliderFlags_ReadOnly);

								ImGui::TreePop();
							}
							p = p->next;
						}
						ImGui::TreePop();
					}
				}

				ImGui::EndDisabled();
				ImGui::EndTable();
			}
		}
		ImGui::PopStyleColor();
		ImGui::EndChild();
	}
	ImGui::End();
}

void CModuleDebugger::SetActive(FAssetClass* ptr)
{
	activeAsset = ptr;
	activeStruct = nullptr;
	activeEnum = nullptr;
}

void CModuleDebugger::SetActive(FStruct* ptr)
{
	activeAsset = nullptr;
	activeStruct = ptr;
	activeEnum = nullptr;
}

void CModuleDebugger::SetActive(FEnum* ptr)
{
	activeAsset = nullptr;
	activeStruct = nullptr;
	activeEnum = ptr;
}
