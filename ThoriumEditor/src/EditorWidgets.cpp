
#define IMGUI_DEFINE_MATH_OPERATORS
#include "EditorWidgets.h"
#include "Resources/Asset.h"
#include "Resources/ResourceManager.h"
#include "ThemeManager.h"
#include "Platform/Windows/DirectX/DirectXTexture.h"

#include "Math/Math.h"

#define TEX_VIEW(tex) ((DirectXTexture2D*)tex)->view

bool ImGui::ObjectPtrWidget(const char* label, TObjectPtr<CObject>** values, int numValues, FClass* filterClass /*= nullptr*/)
{
	auto& objs = CObjectManager::GetAllObjects();
	bool bEqual = true;

	for (int i = 1; i < numValues; i++)
	{
		if (*values[0] != *values[i])
		{
			bEqual = false;
			break;
		}
	}

	bool bNull = !values[0]->IsValid();

	bool r = false;

	if (ImGui::BeginCombo(label, bEqual ? (bNull ? "None" : (*values[0])->Name().c_str()) : "Multiple Values"))
	{
		if (ImGui::Selectable("None", bNull))
		{
			for (int i = 0; i < numValues; i++)
			{
				*values[i] = nullptr;
				bNull = true;
			}
			r = true;
		}
		if (bNull)
			ImGui::SetItemDefaultFocus();

		for (auto obj : objs)
		{
			if (!obj.second->GetClass()->CanCast(filterClass))
				continue;

			bool bSelected = bEqual ? *values[0] == obj.second : false;
			if (ImGui::Selectable((obj.second->Name() + "##_" + FString::ToString((SizeType)obj.second)).c_str(), bSelected))
			{
				for (int i = 0; i < numValues; i++)
				{
					*values[i] = obj.second;
				}
				r = true;
			}
			if (bSelected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}
	return r;
}

bool ImGui::AssetPtrWidget(const char* label, TObjectPtr<CAsset>** values, int numValues, FAssetClass* filterClass)
{
	auto& resources = CResourceManager::GetAvailableResources();
	bool bEqual = true;

	for (int i = 1; i < numValues; i++)
	{
		if (*values[0] != *values[i])
		{
			bEqual = false;
			break;
		}
	}

	bool bNull = !values[0]->IsValid();
	bool r = false;

	if (bEqual) {
		auto* img = values[0]->IsValid() ? ThoriumEditor::GetResourceIcon(*values[0]) : nullptr;

		if (!img)
			ImGui::Button("##Preview", ImVec2(48, 48));
		else
			ImGui::ImageButton(TEX_VIEW(img), ImVec2(38, 38));

		ImGui::SameLine();
	}

	ImVec2 p = ImGui::GetCursorScreenPos();

	ImGui::SetNextItemWidth(FMath::Max(ImGui::GetContentRegionAvail().x - 61.f, 60.f));

	if (ImGui::BeginCombo((FString("##_") + label).c_str(), bEqual ? (bNull ? "None" : ((*values[0])->File() ? (*values[0])->File()->Name().c_str() : (*values[0])->Name().c_str())) : "Multiple Values"))
	{
		if (ImGui::Selectable("None", bNull))
		{
			for (int i = 0; i < numValues; i++)
			{
				*values[i] = nullptr;
				bNull = true;
			}
			r = true;
		}
		if (bNull)
			ImGui::SetItemDefaultFocus();

		for (auto& obj : resources)
		{
			if (!obj.second.type->CanCast(filterClass))
				continue;

			bool bSelected = bEqual ? (bNull ? false : (*values[0])->File() == obj.second.file) : false;
			if (ImGui::Selectable(obj.second.file->Name().c_str(), bSelected))
			{
				TObjectPtr<CAsset> resource = CResourceManager::GetResource(filterClass, obj.first);
				for (int i = 0; i < numValues; i++)
				{
					*values[i] = resource;
				}
				r = true;
			}
			if (bSelected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	if (ImGui::BeginDragDropTarget())
	{
		const ImGuiPayload* content = ImGui::AcceptDragDropPayload("THORIUM_ASSET_FILE");
		if (content)
		{
			FFile* file = *(FFile**)content->Data;
			FAssetClass* type = CResourceManager::GetResourceTypeByFile(file);
			if (type->CanCast(filterClass))
			{
				TObjectPtr<CAsset> resource = CResourceManager::GetResource(filterClass, file->Path());
				for (int i = 0; i < numValues; i++)
				{
					*values[i] = resource;
				}
			}
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::SameLine();
	ImGui::Button("Browse");

	ImGui::SetCursorScreenPos(p + ImVec2(0, 28));
	FString l = label;
	if (auto i = l.Find("##"); i != -1)
		l.Erase(l.begin() + i, l.end());

	ImGui::Text(l.c_str());

	return r;
}

bool ImGui::ClassPtrWidget(const char* label, TClassPtr<CObject>** values, int numValues, FClass* filterClass /*= nullptr*/)
{
	bool bEqual = true;

	for (int i = 1; i < numValues; i++)
	{
		if (*values[0] != *values[i])
		{
			bEqual = false;
			break;
		}
	}

	bool bNull = values[0]->Get() == nullptr;

	bool r = false;

	if (ImGui::BeginCombo(label, bEqual ? (bNull ? "None" : (*values[0]).Get()->GetName().c_str()) : "Multiple Values"))
	{
		static TArray<FClass*> classes;
		classes.Clear();
		classes.Add(filterClass);
		CModuleManager::FindChildClasses(filterClass, classes);

		// Don't allow null values
		//if (ImGui::Selectable("None", bNull))
		//{
		//	for (int i = 0; i < numValues; i++)
		//	{
		//		*values[i] = nullptr;
		//	}
		//	r = true;
		//}
		//if (bNull)
		//	ImGui::SetItemDefaultFocus();

		for (auto* c : classes)
		{
			bool bSelected = bEqual ? *values[0] == c : false;
			if (ImGui::Selectable(c->GetName().c_str(), bSelected))
			{
				for (int i = 0; i < numValues; i++)
				{
					*values[i] = c;
				}
				r = true;
			}
			if (bSelected)
				ImGui:SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	return r;
}
