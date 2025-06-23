
#include "IntEdit.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"

CIntEdit::CIntEdit(int numObjects, CObject** object, void** data, const FProperty* property) : IPropertyEditor(numObjects, object, data, property)
{
}

void CIntEdit::Render()
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 4));
	ImGui::Text(property->name.c_str());
	ImGui::TableNextColumn();

	FString propId = FString::ToString((SizeType)property + (SizeType)objects[0]);
	bool bReadOnly = (property->flags & VTAG_EDITOR_EDITABLE) == 0;

	if (property->type == EVT_UINT)
	{
		if (objects.Size() > 1)
		{
		}
		else
		{
			switch (property->size)
			{
			case 1:
			{
				uint8* v = (uint8*)(SizeType)objects[0];
				int _v = *v;

				if (ImGui::DragInt(("##_dragInt" + propId).c_str(), &_v, 1, 0, UINT8_MAX, "%d", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
				{
					*v = _v;
					Validate();
				}
			}
			break;
			case 2:
			{
				uint16* v = (uint16*)(SizeType)objects[0];
				int _v = *v;

				if (ImGui::DragInt(("##_dragInt" + propId).c_str(), &_v, 1, 0, UINT16_MAX, "%d", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
				{
					*v = _v;
					Validate();
				}
			}
			break;
			case 4:
			{
				uint32* v = (uint32*)(SizeType)objects[0];
				int _v = *v;

				if (ImGui::DragInt(("##_dragInt" + propId).c_str(), &_v, 1, 0, UINT_MAX, "%d", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
				{
					*v = _v;
					Validate();
				}
			}
			break;
			case 8:
			{
				uint64* v = (uint64*)(SizeType)objects[0];
				int _v = (int)*v;

				if (ImGui::DragInt(("##_dragInt" + propId).c_str(), &_v, 1, 0, UINT_MAX, "%d", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
				{
					*v = _v;
					Validate();
				}
			}
			break;
			}
		}
	}
	else
	{
		if (objects.Size() > 1)
		{
		}
		else
		{
			switch (property->size)
			{
			case 1:
			{
				int8* v = (int8*)(SizeType)objects[0];
				int _v = *v;

				if (ImGui::DragInt(("##_dragInt" + propId).c_str(), &_v, 1, 0, 0, "%d", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
				{
					*v = _v;
					Validate();
				}
			}
			break;
			case 2:
			{
				int16* v = (int16*)(SizeType)objects[0];
				int _v = *v;

				if (ImGui::DragInt(("##_dragInt" + propId).c_str(), &_v, 1, 0, 0, "%d", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
				{
					*v = _v;
					Validate();
				}
			}
			break;
			case 4:
			{
				int32* v = (int32*)(SizeType)objects[0];
				int _v = *v;

				if (ImGui::DragInt(("##_dragInt" + propId).c_str(), &_v, 1, 0, 0, "%d", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
				{
					*v = _v;
					Validate();
				}
			}
			break;
			case 8:
			{
				int64* v = (int64*)(SizeType)objects[0];
				int _v = (int)*v;

				if (ImGui::DragInt(("##_dragInt" + propId).c_str(), &_v, 1, 0, 0, "%d", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
				{
					*v = _v;
					Validate();
				}
			}
			break;
			}
		}
	}
}
