
#include "StructEdit.h"
#include "Object/Object.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"

CStructEdit::CStructEdit(int numObjects, CObject** object, void** data, const FProperty* property) : IPropertyEditor(numObjects, object, data, property)
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

void CStructEdit::Render()
{
	FString propId = FString::ToString((SizeType)property + (SizeType)objects[0]);
	if (ImGui::TableTreeHeader((property->name + "##" + propId).c_str(), 0, true))
	{
		for (auto& edit : editors)
		{
			edit->Render();
		}

		ImGui::TreePop();
	}
}
