
#include "FloatEdit.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"

CFloatEdit::CFloatEdit(int numObjects, CObject** object, void** data, const FProperty* property) : IPropertyEditor(numObjects, object, data, property)
{
}

void CFloatEdit::Render()
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
	}
	else
	{
		switch (property->size)
		{
		case 4:
		{
			float* v = (float*)objects[0];
			float _v = *v;

			if (ImGui::DragFloat(("##_dragInt" + propId).c_str(), &_v, 1, 0, 0, "%.3f", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
			{
				*v = _v;
				Validate();
			}
		}
		break;
		case 8:
		{
			double* v = (double*)objects[0];
			float _v = (float)*v;

			if (ImGui::DragFloat(("##_dragInt" + propId).c_str(), &_v, 1, 0, 0, "%.3f", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
			{
				*v = (double)_v;
				Validate();
			}
		}
		break;
		}
	}
}
