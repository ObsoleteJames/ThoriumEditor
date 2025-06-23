
#include "BoolEdit.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"

CBoolEdit::CBoolEdit(int numObjects, CObject** object, void** data, const FProperty* property) : IPropertyEditor(numObjects, object, data, property)
{

}

void CBoolEdit::Render()
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 4));
	ImGui::Text(property->name.c_str());
	ImGui::TableNextColumn();

	FString propId = FString::ToString((SizeType)property + (SizeType)objects[0]);
	bool bReadOnly = (property->flags & VTAG_EDITOR_EDITABLE) == 0;
	
	if (objects.Size() > 1)
	{
		bool* value = (bool*)(((SizeType)objects[0]));
		bool bDifferent = false;

		for (int i = 1; i < objects.Size(); i++)
		{
			bool v = *(bool*)(((SizeType)objects[i]));
			if (v != *value)
			{
				bDifferent = true;
				break;
			}
		}

		if (bDifferent)
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);

		if (ImGui::Checkbox(("##_checkbox" + propId).c_str(), value))
		{
			for (int i = 0; i < objects.Size(); i++)
			{
				bool& _v = *(bool*)(((SizeType)objects[i]));
				_v = *value;

				Validate();
			}
		}

		if (bDifferent)
			ImGui::PopItemFlag();
	}
	else
	{
		bool* v = (bool*)(((SizeType)objects[0]));
		ImGui::Checkbox(("##_checkbox" + propId).c_str(), v);
	}
}
