
#include "ObjectPtrEdit.h"

#include "Object/Object.h"
#include "Assets/Asset.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "EditorWidgets.h"

CObjectPtrEdit::CObjectPtrEdit(int numObjects, CObject** object, void** data, const FProperty* property) : IPropertyEditor(numObjects, object, data, property)
{
}

void CObjectPtrEdit::Render()
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 4));
	ImGui::Text(property->name.c_str());
	ImGui::TableNextColumn();

	FString propId = FString::ToString((SizeType)property + (SizeType)objects[0]);
	bool bReadOnly = (property->flags & VTAG_EDITOR_EDITABLE) == 0;

	//TObjectPtr<CObject>* _obj = objects.Size() == 1 ? (TObjectPtr<CObject>*)(SizeType)&objects[0] : nullptr;
	
	static TArray<TObjectPtr<CObject>*> objs;
	objs.Clear();

	for (int i = 0; i < objects.Size(); i++)
		objs.Add((TObjectPtr<CObject>*)(SizeType)objects[i]);

	FClass* type = CModuleManager::FindClass(property->typeName);

	if (type && type->CanCast(CAsset::StaticClass()))
	{
		if (ImGui::AssetPtrWidget(("##_assetPtr" + propId).c_str(), (TObjectPtr<CAsset>**)objs.Data(), (int)objs.Size(), (FAssetClass*)type))
			Validate();
	}
	else
		if (ImGui::ObjectPtrWidget(("##_objectPtr" + propId).c_str(), objs.Data(), (int)objs.Size(), type))
			Validate();
}
