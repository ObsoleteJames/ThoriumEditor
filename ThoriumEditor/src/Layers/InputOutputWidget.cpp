
#include <string>
#include "InputOutputWidget.h"
#include "Game/Entity.h"
#include "EditorEngine.h"

#include "Math/Color.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "EditorWidgets.h"

void CInputOutputWidget::OnUIRender()
{
	auto& selectedEntities = gEditorEngine()->selectedEntities;
	CEntity* ent = selectedEntities.Size() == 1 ? selectedEntities[0] : nullptr;

	if (prevEnt != ent)
	{
		prevEnt = ent;
		selectedOutput = -1;
	}

	if (ImGui::Begin("Input/Output##_editorIOWidget", &bEnabled))
	{
		constexpr ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg;
		if (ImGui::BeginTable("_entIOBindingsTable", 4, flags))
		{
			ImGui::TableSetupColumn("Output", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("Target");
			ImGui::TableSetupColumn("Input");
			ImGui::TableSetupColumn("Delay");
			ImGui::TableHeadersRow();

			if (ent)
			{
				auto& outputs = ent->GetOutputs();
				for (auto i = 0; i < outputs.Size(); i++)
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					FString id = outputs[i].outputName + "##_entOutput" + FString::ToString(i);
					if (ImGui::Selectable(id.c_str(), selectedOutput == i, ImGuiSelectableFlags_SpanAllColumns))
						selectedOutput = i;

					if (!gWorld->IsActive() && ImGui::BeginPopupContextWindow(id.c_str()))
					{
						if (ImGui::MenuItem("Remove"))
						{
							TArray<FOutputBinding>& o = *(TArray<FOutputBinding>*)&outputs;
							o.Erase(o.At(i));

							if (selectedOutput == i)
								selectedOutput = -1;
						}

						ImGui::EndPopup();
					}

					//ImGui::Text(outputs[i].outputName);
					ImGui::TableNextColumn();

					TObjectPtr<CEntity> targetEnt = outputs[i].targetObject.GetAs<CEntity>();
					if (targetEnt)
						ImGui::Text(targetEnt->Name());
					else
						ImGui::Text("null");

					ImGui::TableNextColumn();
					ImGui::Text(outputs[i].targetInput);
					ImGui::TableNextColumn();
					ImGui::Text(std::to_string(outputs[i].delay).c_str());
				}
			}

			ImGui::EndTable();
		}
		if (ImGui::Button("Add") && ent)
			ent->AddOutput();
		
		if (selectedOutput != -1 && ent)
		{
			FOutputBinding& output = *(FOutputBinding*)&ent->GetOutput(selectedOutput);

			if (ImGui::BeginCombo("Output", output.outputName.c_str()))
			{
				for (auto* c = ent->GetClass(); c != nullptr; c = c->GetBaseClass())
				{
					const FFunction* func = c->GetFunctionList();
					for (; func != nullptr; func = func->next)
					{
						if (func->type != FFunction::OUTPUT)
							continue;

						if (ImGui::Selectable(func->name.c_str(), output.outputName == func->name))
							output.outputName = func->name;
					}
				}

				ImGui::EndCombo();
			}

			TObjectPtr<CEntity> targetEnt = output.targetObject.GetAs<CEntity>();

			TObjectPtr<CObject>* targetPtr = (TObjectPtr<CObject>*)&targetEnt;
			if (ImGui::ObjectPtrWidget("Target", &targetPtr, 1, CEntity::StaticClass()))
				output.targetObject = targetEnt;

			if (ImGui::BeginCombo("Input", output.targetInput.c_str()))
			{
				if (targetEnt)
				{
					for (auto* c = targetEnt->GetClass(); c != nullptr; c = c->GetBaseClass())
					{
						const FFunction* func = c->GetFunctionList();
						for (; func != nullptr; func = func->next)
						{
							if (func->type != FFunction::GENERAL || ((int)func->flags & FunctionFlags_ALLOW_AS_INPUT) == 0)
								continue;

							if (ImGui::Selectable(func->name.c_str(), output.targetInput == func->name))
								output.targetInput = func->name;
						}
					}
				}

				ImGui::EndCombo();
			}

			ImGui::DragFloat("Delay", &output.delay, 0.1f, 0.f, 10000.f);
			ImGui::SameLine();
			ImGui::Checkbox("Only Once", &output.bOnlyOnce);
		}
	}
	ImGui::End();
}
