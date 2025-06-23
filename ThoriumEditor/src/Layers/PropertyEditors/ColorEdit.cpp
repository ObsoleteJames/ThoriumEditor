
#include "ColorEdit.h"
#include "Object/Object.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"

CColor4Edit::CColor4Edit(int numObjects, CObject** object, void** data, const FProperty* property) : IPropertyEditor(numObjects, object, data, property)
{
	FStruct* type = CModuleManager::FindStruct(property->typeName);
	if (!type)
		return;

	for (const FProperty* p = type->GetPropertyList(); p != nullptr; p = p->next)
	{
		if ((p->flags & VTAG_EDITOR_VISIBLE) == 0 && (p->flags & VTAG_EDITOR_EDITABLE) == 0)
			continue;

		editors.Add(IPropertyEditor::GetEditor(numObjects, owners.Data(), objects.Data(), p));
	}
}

void CColor4Edit::Render()
{
	SizeType propId = (SizeType)property + (SizeType)objects[0];

	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_TableRowBg]);

	ImGui::PushID(propId);
	bool bOpen = ImGui::TreeNodeEx(property->name.c_str(), ImGuiTreeNodeFlags_Framed);
	ImGui::PopID();
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();

	if (objects.Size() < 2)
	{
		ImGui::PushID(propId);
		ImGui::ColorEdit4("##colorEdit", (float*)objects[0]);
		ImGui::PopID();
	}

	if (bOpen)
	{
		for (auto& edit : editors)
			edit->Render();

		ImGui::TreePop();
	}
}

CColor3Edit::CColor3Edit(int numObjects, CObject** object, void** data, const FProperty* property) : IPropertyEditor(numObjects, object, data, property)
{
	FStruct* type = CModuleManager::FindStruct(property->typeName);
	if (!type)
		return;

	for (const FProperty* p = type->GetPropertyList(); p != nullptr; p = p->next)
	{
		if ((p->flags & VTAG_EDITOR_VISIBLE) == 0 && (p->flags & VTAG_EDITOR_EDITABLE) == 0)
			continue;

		editors.Add(IPropertyEditor::GetEditor(numObjects, owners.Data(), objects.Data(), p));
	}
}

void CColor3Edit::Render()
{
	SizeType propId = (SizeType)property + (SizeType)objects[0];
	
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_TableRowBg]);

	ImGui::PushID(propId);
	bool bOpen = ImGui::TreeNodeEx(property->name.c_str(), ImGuiTreeNodeFlags_Framed);
	ImGui::PopID();
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();

	if (objects.Size() < 2)
	{
		ImGui::PushID(propId);
		ImGui::ColorEdit3("##colorEdit", (float*)objects[0]);
		ImGui::PopID();
	}

	if (bOpen)
	{
		for (auto& edit : editors)
			edit->Render();

		ImGui::TreePop();
	}
}
