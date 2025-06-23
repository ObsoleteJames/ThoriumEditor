
#include "EnumEdit.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"

CEnumEdit::CEnumEdit(int numObjects, CObject** object, void** data, const FProperty* property) : IPropertyEditor(numObjects, object, data, property)
{

}

void CEnumEdit::Render()
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 4));
	ImGui::Text(property->name.c_str());
	ImGui::TableNextColumn();

	FString propId = FString::ToString((SizeType)property + (SizeType)objects[0]);
	bool bReadOnly = (property->flags & VTAG_EDITOR_EDITABLE) == 0;

	FEnum* _enum = CModuleManager::FindEnum(property->typeName);
	if (!_enum)
		return;

	void* value = (void*)(((SizeType)objects[0]));
	int64 v;
	switch (_enum->Size())
	{
	case 1:
		v = *(int8*)value;
		break;
	case 2:
		v = *(int16*)value;
		break;
	case 4:
		v = *(int32*)value;
		break;
	case 8:
		v = *(int64*)value;
		break;
	}

	FString name = _enum->GetNameByValue(v);

	if (objects.Size() > 1)
	{
		bool bDifferent = false;

		for (int i = 1; i < objects.Size(); i++)
		{
			void* _v = (void*)(((SizeType)objects[i]));

			if (_enum->Size() == 1 && *(int8*)_v != v)
			{
				bDifferent = true;
				break;
			}
			else if (_enum->Size() == 2 && *(int16*)_v != v)
			{
				bDifferent = true;
				break;
			}
			else if (_enum->Size() == 4 && *(int32*)_v != v)
			{
				bDifferent = true;
				break;
			}
			else if (_enum->Size() == 8 && *(int64*)_v != v)
			{
				bDifferent = true;
				break;
			}
		}

		if (ImGui::BeginCombo(("##_combo" + propId).c_str(), bDifferent ? "Multiple Values" : name.c_str()))
		{
			for (auto& val : _enum->GetValues())
			{
				bool bSelected = bDifferent ? false : val.Value == v;
				if (ImGui::Selectable(val.Key.c_str(), bSelected))
				{
					for (int i = 0; i < objects.Size(); i++)
					{
						void* _v = (void*)(((SizeType)objects[i]));
						v = val.Value;
						switch (_enum->Size())
						{
						case 1:
							*(int8*)_v = (int8)v;
							break;
						case 2:
							*(int16*)_v = (int16)v;
							break;
						case 4:
							*(int32*)_v = (int32)v;
							break;
						case 8:
							*(int64*)_v = (int64)v;
							break;
						}
					}
				}
				if (bSelected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
	}
	else
	{
		if (ImGui::BeginCombo(("##_combo" + propId).c_str(), name.c_str()))
		{
			for (auto& val : _enum->GetValues())
			{
				bool bSelected = val.Value == v;
				if (ImGui::Selectable(val.Key.c_str(), bSelected))
				{
					v = val.Value;
					switch (_enum->Size())
					{
					case 1:
						*(int8*)value = (int8)v;
						break;
					case 2:
						*(int16*)value = (int16)v;
						break;
					case 4:
						*(int32*)value = (int32)v;
						break;
					case 8:
						*(int64*)value = (int64)v;
						break;
					}

					Validate();
				}
				if (bSelected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
	}
}
