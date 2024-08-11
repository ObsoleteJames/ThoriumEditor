
#include <string>
#include "PropertyEditor.h"
#include "Game/Entity.h"
#include "EditorEngine.h"
#include "Game/Components/SceneComponent.h"

#include "Math/Color.h"

#include <map>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"
#include "EditorWidgets.h"
#include "EditorMenu.h"

#include "ClassSelectorPopup.h"

REGISTER_EDITOR_LAYER(CPropertyEditor, "View/Properties", nullptr, false, true)

void CPropertyEditor::OnUIRender()
{
	auto& selectedEntities = gEditorEngine()->selectedEntities;

	// Property Editor
	if (ImGui::Begin("Properties##_editorPropertyEditor", &bEnabled))
	{
		char editName[65];
		if (selectedEntities.Size() == 1)
			memcpy(editName, selectedEntities[0]->Name().c_str(), FMath::Min(selectedEntities[0]->Name().Size() + 1, 64ull));
		else if (selectedEntities.Size() > 1)
			memcpy(editName, "Multiple Selected", 18);
		else
			editName[0] = '\0';

		ImVec2 cursor = ImGui::GetCursorScreenPos();
		ImVec2 region = ImGui::GetContentRegionAvail();

		ImGui::BeginDisabled(selectedEntities.Size() == 0);

		if (ImGui::InputText("Name##_editorPropertyEditor", editName, 64))
		{
			selectedEntities[0]->SetName(editName);
		}

		ImGui::SetCursorScreenPos(cursor + ImVec2(region.x - 38, 0));
		ImGui::ButtonClear("Add", ImVec2(32, 32));

		{
			FClass* compClass = nullptr;
			if (ThoriumEditor::AcceptClass("PropertyEditorAddComponent", &compClass) && compClass)
				AddComponent(compClass);
		}
		
		if (ImGui::BeginPopupContextItem("addCompBtn", ImGuiPopupFlags_MouseButtonLeft))
		{
			// the class to add.
			FString c;

			if (ImGui::BeginMenu("Lights"))
			{
				if (ImGui::MenuItem("Point Light"))
					c = "CPointLightComponent";
				if (ImGui::MenuItem("Spot Light"));
				if (ImGui::MenuItem("Sun Light"))
					c = "CSunLightComponent";
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Volumes"))
			{
				if (ImGui::MenuItem("Post Process Volume"))
					c = "CPostProcessVolumeComp";
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Scene Component"))
				c = "CSceneComponent";
			if (ImGui::MenuItem("Model"))
				c = "CModelComponent";
			if (ImGui::MenuItem("Camera"))
				c = "CCameraComponent";

			ImGui::Separator();

			if (ImGui::MenuItem("Select Class..."))
				ThoriumEditor::SelectClass("PropertyEditorAddComponent", CEntityComponent::StaticClass());

			ImGui::EndPopup();

			FClass* _class = c.IsEmpty() ? nullptr : CModuleManager::FindClass(c);
			if (_class)
				AddComponent(_class);
		}

		ImGui::EndDisabled();

		if (selectedEntities.Size() == 1)
		{
			if (selectedEntities[0] != prevEnt)
			{
				prevEnt = selectedEntities[0];
				rotCache = prevEnt->RootComponent()->GetRotation().ToEuler().Degrees();
				selectedComp = nullptr;
			}

			if (ImGui::BeginTable("entityComponentsEdit", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(0, 48)))
			{
				// TODO: this idfk
				struct FComponentTree {
					TArray<FComponentTree> childTrees;
					FComponentTree* parent;
					TObjectPtr<CEntityComponent> comp;

					FComponentTree* AddChild(CEntityComponent* comp) {
						childTrees.Add();
						auto* t = &*childTrees.last();
						t->parent = this;
						t->comp = comp;
						return t;
					}
					FComponentTree* AddChild(CSceneComponent* comp) {
						childTrees.Add();
						auto* t = &*childTrees.last();
						t->parent = this;
						t->comp = comp;

						for (TObjectPtr<CSceneComponent>& c : comp->GetChildren())
						{
							if (c && c->GetEntity() == comp->GetEntity())
								t->AddChild(c);
						}

						return t;
					}
					FComponentTree* FindTree(CEntityComponent* c) 
					{
						if (c == comp)
							return this;

						for (auto& child : childTrees)
						{
							if (auto* t = child.FindTree(c); t != nullptr)
								return t;
						}

						return nullptr;
					}

					void DrawUI(CEntityComponent*& selectedComp, FVector& rotCache)
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();

						bool bSelected = selectedComp == comp;

						if (!bSelected)
							ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
						else
							ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.21f, 0.26f, 0.38f, 1.00f));

						if (ImGui::Selectable(("##_compSelect" + comp->Name() + FString::ToString((SizeType)&*comp)).c_str(), bSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
						{
							selectedComp = comp;

							if (auto sc = Cast<CSceneComponent>(comp); sc)
								rotCache = sc->GetRotation().ToEuler().Degrees();
						}
						if (ImGui::BeginDragDropSource())
						{
							ImGui::SetDragDropPayload("ENTITY_COMPONENT_PAYLOAD", &comp, sizeof(void*));
							ImGui::EndDragDropSource();
						}

						if (comp->IsUserCreated() && ImGui::BeginPopupContextItem())
						{
							if (ImGui::MenuItem("Delete"))
							{
								comp->GetEntity()->RemoveComponent(comp);
							}

							ImGui::EndPopup();
						}

						if (ImGui::BeginDragDropTarget())
						{
							const auto* payload = ImGui::AcceptDragDropPayload("ENTITY_COMPONENT_PAYLOAD");
							if (payload)
							{
								TObjectPtr<CEntityComponent> c = *(CEntityComponent**)payload->Data;
								TObjectPtr<CSceneComponent> cScene = CastChecked<CSceneComponent>(c);
								TObjectPtr<CSceneComponent> compScene = Cast<CSceneComponent>(comp);
								if (c != comp && cScene && compScene)
								{
									cScene->AttachTo(compScene);
								}
							}

							ImGui::EndDragDropTarget();
						}

						ImGui::PopStyleColor();
						ImGui::SameLine();

						ImVec2 cursor = ImGui::GetCursorScreenPos();
						FString compName = comp->Name().IsEmpty() ? comp->GetClass()->GetName() : comp->Name();
						if (childTrees.Size() > 0)
						{
							ImGui::SetNextItemWidth(10);
							ImGui::SetCursorScreenPos(cursor - ImVec2(14, 0));
							bool bOpen = ImGui::TreeNodeEx(("##_treeComp" + comp->Name()).c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen);
							ImGui::SameLine();

							ImGui::SetCursorScreenPos(cursor + ImVec2(6, 0));
							ImGui::Text(compName.c_str());
							
							if (bOpen)
							{
								for (auto& c : childTrees)
								{
									c.DrawUI(selectedComp, rotCache);
								}
								ImGui::TreePop();
							}
						}
						else
						{
							ImGui::SetCursorScreenPos(cursor + ImVec2(6, 0));
							ImGui::Text(compName.c_str());
						}
					}
				};

				static FComponentTree compTree;
				compTree.childTrees.Clear();
				
				static TArray<CEntityComponent*> nonSceneComps;
				nonSceneComps.Clear();

				for (auto& comp : selectedEntities[0]->GetAllComponents())
				{
					if (auto scene = CastChecked<CSceneComponent>(comp.second); scene != nullptr)
					{
						if (scene->GetParent() == nullptr && scene != selectedEntities[0]->RootComponent())
						{
							nonSceneComps.Add(comp.second);
						}
						if (scene->GetParent())
						{
							bool bInvalid = true;
							for (auto it : scene->GetParent()->GetChildren())
							{
								if (it == scene)
								{
									bInvalid = false;
									break;
								}
							}
							if (bInvalid)
								nonSceneComps.Add(comp.second);
						}
					}
					else
					{
						nonSceneComps.Add(comp.second);
					}
				}

				if (selectedEntities[0]->RootComponent())
				{
					for (TObjectPtr<CSceneComponent>& c : selectedEntities[0]->RootComponent()->GetChildren())
						if (c)
							compTree.AddChild(c);
				}

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				bool bSelected = selectedComp == nullptr;

				if (!bSelected)
					ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
				else
					ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.21f, 0.26f, 0.38f, 1.00f));

				if (ImGui::Selectable(("##_compSelect" + selectedEntities[0]->Name()).c_str(), bSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
				{
					selectedComp = nullptr;
				}
				if (ImGui::BeginDragDropTarget())
				{
					const auto* payload = ImGui::AcceptDragDropPayload("ENTITY_COMPONENT_PAYLOAD");
					if (payload)
					{
						TObjectPtr<CEntityComponent> c = *(CEntityComponent**)payload->Data;
						TObjectPtr<CSceneComponent> cScene = CastChecked<CSceneComponent>(c);
						if (cScene)
						{
							cScene->AttachTo(selectedEntities[0]->RootComponent());
						}
					}

					ImGui::EndDragDropTarget();
				}

				ImGui::PopStyleColor();
				ImGui::SameLine();

				ImVec2 cursor = ImGui::GetCursorScreenPos();

				ImGui::SetNextItemWidth(10);
				ImGui::SetCursorScreenPos(cursor - ImVec2(14, 0));
				bool bOpen = ImGui::TreeNodeEx(("##_treeEnt" + selectedEntities[0]->Name()).c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen);
				ImGui::SameLine();

				ImGui::SetCursorScreenPos(cursor + ImVec2(6, 0));
				ImGui::Text(selectedEntities[0]->Name().c_str());

				if (bOpen)
				{
					for (auto& c : compTree.childTrees)
						c.DrawUI(selectedComp, rotCache);

					ImGui::TreePop();
				}

				if (nonSceneComps.Size() > 0)
					ImGui::Separator();

				for (auto& c : nonSceneComps)
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					bool bSelected = selectedComp == c;

					if (!bSelected)
						ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
					else
						ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.21f, 0.26f, 0.38f, 1.00f));

					if (ImGui::Selectable(("##_compSelect" + c->Name() + FString::ToString((SizeType)c)).c_str(), bSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
					{
						selectedComp = c;
					}

					CSceneComponent* sceneComp = Cast<CSceneComponent>(TObjectPtr<CEntityComponent>(c));
					if (sceneComp && ImGui::BeginPopupContextItem())
					{
						if (ImGui::MenuItem("Attach to entity root"))
							sceneComp->AttachTo(c->GetEntity()->RootComponent(), FTransformSpace::KEEP_LOCAL_TRANSFORM);

						ImGui::EndPopup();
					}

					ImGui::PopStyleColor();
					ImGui::SameLine();

					ImVec2 cursor = ImGui::GetCursorScreenPos();
					ImGui::SetCursorScreenPos(cursor + ImVec2(6, 0));
					ImGui::Text(c->Name().c_str());
				}

				ImGui::EndTable();
			}
		}
		else
		{
			prevEnt = nullptr;
			selectedComp = nullptr;
		}

		if (selectedEntities.Size() > 0)
		{
			int type = selectedEntities[0]->GetType() - 1;
			const char* typeNames[] = { "Static", "Dynamic" };
			if (ImGui::TypeSelector("##_entTypeSwitch", &type, 2, typeNames))
			{
				for (auto& ent : selectedEntities)
				{
					if (type == 0)
						ent->MakeStatic();
					else if (type == 1)
						ent->MakeDynamic();
				}
					//ent->type = (EEntityType)(type + 1);
			}

			static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
			if (ImGui::BeginTable("propertyEditorTable", 2, flags))
			{
				FClass* _class = selectedEntities[0]->GetClass();
				for (int i = 1; i < selectedEntities.Size(); i++)
				{
					if (selectedEntities[i]->GetClass() != _class)
					{
						// Fallback to entity class if all selected objects aren't the same type
						_class = CEntity::StaticClass();
						break;
					}
				}

				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Value");
				ImGui::TableHeadersRow();

				_class = selectedComp ? selectedComp->GetClass() : _class;

				if (_class->CanCast(CEntity::StaticClass()) || _class->CanCast(CSceneComponent::StaticClass()))
					RenderTransformEdit();

				RenderClassProperties(_class, 0);
				ImGui::EndTable();
			}
		}
	}
	ImGui::End();
	Menu()->bChecked = bEnabled;
}

void CPropertyEditor::RenderClassProperties(FStruct* type, SizeType offset, bool bHeader)
{
	auto& selectedEntities = gEditorEngine()->selectedEntities;
	std::multimap<std::string, const FProperty*> propertyPerCategory;

	FClass* _class = type->IsClass() ? (FClass*)type : nullptr;
	for (FClass* c = _class; c != nullptr; c = c->GetBaseClass())
	{
		for (const FProperty* p = c->GetPropertyList(); p != nullptr; p = p->next)
		{
			if ((p->flags & VTAG_EDITOR_VISIBLE) == 0 && (p->flags & VTAG_EDITOR_EDITABLE) == 0)
				continue;

			if (p->meta && !p->meta->category.IsEmpty())
				propertyPerCategory.insert(std::make_pair(p->meta->category.c_str(), p));
			else
				propertyPerCategory.insert(std::make_pair(c->GetName().c_str(), p));
		}
	}
	if (!_class)
	{
		for (const FProperty* p = type->GetPropertyList(); p != nullptr; p = p->next)
		{
			if ((p->flags & VTAG_EDITOR_VISIBLE) == 0 && (p->flags & VTAG_EDITOR_EDITABLE) == 0)
				continue;

			if (p->meta && !p->meta->category.IsEmpty())
				propertyPerCategory.insert(std::make_pair(p->meta->category.c_str(), p));
			else
				propertyPerCategory.insert(std::make_pair(type->GetName().c_str(), p));
		}
	}

	ImVec4* colors = ImGui::GetStyle().Colors;

	FString curCat = propertyPerCategory.size() > 0 ? propertyPerCategory.rbegin()->first : "NULL";
	ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FrameDontExpand;

	//ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(5, 0));
	//ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 7));
	//ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
	//ImGui::TableNextRow();
	//ImGui::TableNextColumn();

	//bool bOpen = ImGui::TreeNodeEx(curCat.c_str(), treeFlags);
	//ImGui::PopStyleVar(3);
	bool bOpen = bHeader ? ImGui::TableTreeHeader(curCat.c_str(), treeFlags) : false;
	
	for (auto it = propertyPerCategory.rbegin(); it != propertyPerCategory.rend(); it++)
	{
		bool bReadOnly = (it->second->flags & VTAG_EDITOR_EDITABLE) == 0;
		if (curCat != it->first)
		{
			curCat = it->first;
			if (bOpen)
				ImGui::TreePop();

			bOpen = ImGui::TableTreeHeader(curCat.c_str(), treeFlags);
		}

		if (!bOpen && bHeader)
			continue;

		const FProperty* prop = it->second;
		if (prop->type != EVT_ARRAY && prop->type != EVT_STRUCT)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 4));
			ImGui::Text(prop->name.c_str());
			ImGui::TableNextColumn();
		}

		FString propId = FString::ToString((SizeType)prop);

		void* obj = selectedComp != nullptr ? (void*)selectedComp : (void*)&*selectedEntities[0];

		static TArray<void*> objects;
		objects.Clear();

		if (selectedEntities.Size() > 1) 
		{
			for (auto& t : selectedEntities)
				objects.Add((void*)&*t);
		}
		objects.Add(obj);

		RenderProperty(prop->type, prop, objects.Data(), (int)objects.Size(), prop->offset + offset);
	}
	if (bOpen)
		ImGui::TreePop();
}

void CPropertyEditor::RenderProperty(uint type, const FProperty* prop, void** objects, int objCount, SizeType offset)
{
	FString propId = FString::ToString((SizeType)prop + offset);
	bool bReadOnly = (prop->flags & VTAG_EDITOR_EDITABLE) == 0;
	switch (type)
	{
	case EVT_STRUCT:
	{
		FStruct* _struct = CModuleManager::FindStruct(prop->typeName);
		if (!_struct)
			break;

		void* value = (void*)(((SizeType)objects[0]) + offset);
		FString type = (prop->meta && prop->meta->HasFlag("UIType")) ? prop->meta->FlagValue("UIType") : FString();

		if (prop->typeName == "FVector")
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 4));
			ImGui::Text(prop->name.c_str());
			ImGui::TableNextColumn();

			if (type == "Color")
				ImGui::ColorEdit3(("##_ColorEdit" + propId).c_str(), (float*)value);
			else
				RenderVectorProperty(offset, false);
		}
		else
		{
			ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FrameDontExpand;

			if (ImGui::TableTreeHeader(prop->name.c_str(), treeFlags, true))
			{
				RenderClassProperties(_struct, offset, false);
				ImGui::TreePop();
			}
		}
	}
		break;
	case EVT_ENUM:
	{
		FEnum* _enum = CModuleManager::FindEnum(prop->typeName);
		if (!_enum)
			break;

		void* value = (void*)(((SizeType)objects[0]) + offset);
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

		if (objCount > 1)
		{
			bool bDifferent = false;

			for (int i = 1; i < objCount; i++)
			{
				void* _v = (void*)(((SizeType)objects[i]) + offset);

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
						for (int i = 0; i < objCount; i++)
						{
							void* _v = (void*)(((SizeType)objects[i]) + offset);
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
					}
					if (bSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}
		}
	}
	break;
	case EVT_ARRAY:
	{
		if (objCount == 1 && ImGui::TableTreeHeader(prop->name.c_str(), 0, true))
		{
			FArrayHelper* helper = (FArrayHelper*)prop->typeHelper;
			SizeType numElements = helper->Size((void*)((SizeType)objects[0] + offset));

			for (int i = 0; i < numElements; i++)
			{
				void* obj = helper->Data((void*)((SizeType)objects[0] + offset));
				
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text((FString::ToString(i) + ":").c_str());
				ImGui::TableNextColumn();

				RenderProperty(helper->objType, prop, &obj, 1, i * helper->objSize);
			}

			ImGui::TreePop();
		}
	}
	break;
	case EVT_OBJECT_PTR:
	{
		TObjectPtr<CObject>* _obj = objCount == 1 ? (TObjectPtr<CObject>*)((SizeType)objects[0] + offset) : nullptr;
		static TArray<TObjectPtr<CObject>*> objects;
		objects.Clear();
		if (_obj == nullptr)
		{
			for (int i = 0; i < objCount; i++)
			{
				objects.Add((TObjectPtr<CObject>*)((SizeType)objects[i] + offset));
			}
		}
		else
			objects.Add((TObjectPtr<CObject>*)_obj);

		FClass* type = CModuleManager::FindClass(prop->typeName);

		if (!type->CanCast(CAsset::StaticClass()))
			ImGui::ObjectPtrWidget(("##_objectPtr" + propId).c_str(), objects.Data(), (int)objects.Size(), type);
		else
			ImGui::AssetPtrWidget(("##_assetPtr" + propId).c_str(), (TObjectPtr<CAsset>**)objects.Data(), (int)objects.Size(), (FAssetClass*)type);
	}
	break;
	case EVT_STRING:
	{
		FString* str = (FString*)(((SizeType)objects[0]) + offset);
		/*char buff[64];
		memcpy(buff, str->Data(), str->Size() + 1);

		if (ImGui::InputText(("##_inputText" + propId).c_str(), buff, 64, bReadOnly ? ImGuiInputTextFlags_ReadOnly : 0))
			*str = buff;*/
		ImGui::InputText(("##_inputText" + propId).c_str(), str);
	}
	break;
	case EVT_FLOAT:
	{
		float step = (prop->meta && prop->meta->HasFlag("UIStepSize")) ? std::stof(prop->meta->FlagValue("UIStepSize").c_str()) : 0.1f;
		FString format = (prop->meta && prop->meta->HasFlag("UIFormat")) ? prop->meta->FlagValue("UIFormat") : "%.3f";

		if (objCount > 1)
		{
			float* value = (float*)(((SizeType)objects[0]) + offset);
			bool bDifferent = false;

			for (int i = 1; i < objCount; i++)
			{
				float v = *(float*)(((SizeType)objects[i]) + offset);
				if (v != *value)
				{
					bDifferent = true;
					break;
				}
			}

			if (ImGui::DragFloat(("##_floatInput" + propId).c_str(), value, step, 0, 0, bDifferent ? "Multiple Values" : format.c_str(), bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
			{
				for (int i = 0; i < objCount; i++)
				{
					float& _v = *(float*)(((SizeType)objects[i]) + offset);
					_v = *value;
				}
			}
		}
		else
		{
			float* v = (float*)(((SizeType)objects[0]) + offset);
			ImGui::DragFloat(("##_floatInput" + propId).c_str(), v, step, 0, 0, format.c_str(), bReadOnly ? ImGuiSliderFlags_ReadOnly : 0);
		}
	}
	break;
	case EVT_INT:
	{
		if (objCount > 1)
		{

		}
		else
		{
			switch (prop->size)
			{
			case 1:
			{
				int8* v = (int8*)(((SizeType)objects[0]) + offset);
				int _v = *v;

				if (ImGui::DragInt(("##_dragInt" + propId).c_str(), &_v, 1, 0, 0, "%d", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
					*v = _v;

			}
			break;
			case 2:
			{
				int16* v = (int16*)(((SizeType)objects[0]) + offset);
				int _v = *v;

				if (ImGui::DragInt(("##_dragInt" + propId).c_str(), &_v, 1, 0, 0, "%d", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
					*v = _v;

			}
			break;
			case 4:
			{
				int32* v = (int32*)(((SizeType)objects[0]) + offset);
				int _v = *v;

				if (ImGui::DragInt(("##_dragInt" + propId).c_str(), &_v, 1, 0, 0, "%d", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
					*v = _v;

			}
			break;
			case 8:
			{
				int64* v = (int64*)(((SizeType)objects[0]) + offset);
				int _v = (int)*v;

				if (ImGui::DragInt(("##_dragInt" + propId).c_str(), &_v, 1, 0, 0, "%d", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
					*v = _v;

			}
			break;
			}
		}
	}
	break;
	case EVT_BOOL:
	{
		if (objCount > 1)
		{
			bool* value = (bool*)(((SizeType)objects[0]) + offset);
			bool bDifferent = false;

			for (int i = 1; i < objCount; i++)
			{
				bool v = *(bool*)(((SizeType)objects[i]) + offset);
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
				for (int i = 0; i < objCount; i++)
				{
					bool& _v = *(bool*)(((SizeType)objects[i]) + offset);
					_v = *value;
				}
			}

			if (bDifferent)
				ImGui::PopItemFlag();
		}
		else
		{
			bool* v = (bool*)(((SizeType)objects[0]) + offset);
			ImGui::Checkbox(("##_checkbox" + propId).c_str(), v);
		}
	}
	break;
	}
}

void CPropertyEditor::AddComponent(FClass* type)
{
	auto& selectedEntities = gEditorEngine()->selectedEntities;
	TObjectPtr<CEntityComponent> comp = selectedEntities[0]->AddComponent(type, type->GetName());

	// :P
	bool* bUserCreated = (bool*)&*comp + CEntityComponent::__private_bUserCreated_offset();
	*bUserCreated = true;

	if (auto scene = CastChecked<CSceneComponent>(comp); scene)
		scene->AttachTo(selectedEntities[0]->RootComponent());
}

void CPropertyEditor::RenderTransformEdit()
{
	auto& selectedEntities = gEditorEngine()->selectedEntities;
	ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FrameDontExpand;

	bool bOpen = ImGui::TableTreeHeader("Transform", treeFlags);

	if (bOpen)
	{
		if (selectedEntities.Size() > 1)
		{
			CSceneComponent* root = selectedEntities[0]->RootComponent();


		}
		else
		{
			CSceneComponent* scene = selectedComp ? (CSceneComponent*)selectedComp : selectedEntities[0]->RootComponent();

			FVector* p = (FVector*)&scene->GetPosition();
			FVector* s = (FVector*)&scene->GetScale();
			FQuaternion* r = (FQuaternion*)&scene->GetRotation();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 4));
			ImGui::Text("Position");
			ImGui::TableNextColumn();

			CEntityComponent* _c = selectedComp;
			selectedComp = scene;
			if (RenderVectorProperty((SizeType)p - (SizeType)scene, false))
				scene->SetPosition(*p);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 4));
			ImGui::Text("Rotation");
			ImGui::TableNextColumn();
			if (RenderQuatProperty((SizeType)r - (SizeType)scene, false))
				scene->SetPosition(*p); // Doesn't matter what we set, we just need the transform to be updated
			
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 4));
			ImGui::Text("Scale");
			ImGui::TableNextColumn();
			if (RenderVectorProperty((SizeType)s - (SizeType)scene, false))
				scene->SetPosition(*p);

			selectedComp = _c;
		}

		ImGui::TreePop();
	}
}

bool CPropertyEditor::RenderVectorProperty(SizeType offset, bool bReadOnly)
{
	auto& selectedEntities = gEditorEngine()->selectedEntities;
	void* obj = selectedComp != nullptr ? (void*)selectedComp : (void*)&*selectedEntities[0];

	auto areaSize = ImGui::GetContentRegionAvail();
	float tWidth = areaSize.x / 3 - 5.f;

	bool r = false;

	if (selectedEntities.Size() > 1)
	{
		FVector* v = (FVector*)(((SizeType)&*selectedEntities[0]) + offset);
		bool bXDiff = false;
		bool bYDiff = false;
		bool bZDiff = false;

		for (auto& t : selectedEntities)
		{
			FVector& _v = *(FVector*)(((SizeType)&*t) + offset);
			if (v->x != _v.x)
			{
				bXDiff = true;
			}
			if (v->y != _v.y)
			{
				bYDiff = true;
			}
			if (v->z != _v.z)
			{
				bZDiff = true;
			}
		}

		ImGui::SetNextItemWidth(tWidth);
		if (ImGui::DragFloat(("##_vectorInputX" + FString::ToString(offset + (SizeType)v)).c_str(), &v->x, 0.1f, 0, 0, bXDiff ? "Multiple Values" : "X:%.3f", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
		{
			for (auto& t : selectedEntities)
			{
				FVector& _v = *(FVector*)(((SizeType) & *t) + offset);
				_v.x = v->x;
			}
			r = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(tWidth);
		if (ImGui::DragFloat(("##_vectorInputY" + FString::ToString(offset + (SizeType)v)).c_str(), &v->y, 0.1f, 0, 0, bXDiff ? "Multiple Values" : "Y:%.3f", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
		{
			for (auto& t : selectedEntities)
			{
				FVector& _v = *(FVector*)(((SizeType) & *t) + offset);
				_v.y = v->y;
			}
			r = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(tWidth);
		if (ImGui::DragFloat(("##_vectorInputZ" + FString::ToString(offset + (SizeType)v)).c_str(), &v->z, 0.1f, 0, 0, bXDiff ? "Multiple Values" : "Z:%.3f", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
		{
			for (auto& t : selectedEntities)
			{
				FVector& _v = *(FVector*)(((SizeType) & *t) + offset);
				_v.z = v->z;
			}
			r = true;
		}
	}
	else
	{
		FVector* v = (FVector*)(((SizeType)obj) + offset);
		ImGui::SetNextItemWidth(tWidth);
		r = ImGui::DragFloat(("##_vectorInputX" + FString::ToString(offset + (SizeType)v)).c_str(), &v->x, 0.1f, 0, 0, "X:%.3f", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0) || r;
		ImGui::SameLine();
		ImGui::SetNextItemWidth(tWidth);
		r = ImGui::DragFloat(("##_vectorInputY" + FString::ToString(offset + (SizeType)v)).c_str(), &v->y, 0.1f, 0, 0, "Y:%.3f", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0) || r;
		ImGui::SameLine();
		ImGui::SetNextItemWidth(tWidth);
		r = ImGui::DragFloat(("##_vectorInputZ" + FString::ToString(offset + (SizeType)v)).c_str(), &v->z, 0.1f, 0, 0, "Z:%.3f", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0) || r;
	}
	return r;
}

bool CPropertyEditor::RenderColorProperty(SizeType offset, bool bReadOnly)
{
	auto& selectedEntities = gEditorEngine()->selectedEntities;
	void* obj = selectedComp != nullptr ? (void*)selectedComp : (void*)&*selectedEntities[0];

	//auto areaSize = ImGui::GetContentRegionAvail();
	//float tWidth = areaSize.x / 3 - 5.f;

	bool r = false;

	if (selectedEntities.Size() > 1)
	{
		// TODO
	}
	else
	{
		FColor* v = (FColor*)(((SizeType)obj) + offset);
		if (ImGui::ColorEdit4(("##_colorEdit" + FString::ToString(offset + (SizeType)v)).c_str(), (float*)v))
			r = true;
	}
	return r;
}

bool CPropertyEditor::RenderQuatProperty(SizeType offset, bool bReadOnly)
{
	auto& selectedEntities = gEditorEngine()->selectedEntities;
	void* obj = selectedComp != nullptr ? (void*)selectedComp : (void*)&*selectedEntities[0];

	auto areaSize = ImGui::GetContentRegionAvail();
	float tWidth = areaSize.x / 3 - 5.f;

	bool r;

	if (selectedEntities.Size() > 1)
	{

	}
	else
	{
		FQuaternion* v = (FQuaternion*)(((SizeType)obj) + offset);
		FVector euler = v->ToEuler().Degrees();
		FVector prev = euler;

		bool bUpdate = false;
		int axis = 0;

		ImGui::SetNextItemWidth(tWidth);
		if (ImGui::DragFloat(("##_vectorInputX" + FString::ToString(offset + (SizeType)v)).c_str(), &euler.x, 0.1f, 0, 0, "X:%.3f", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
		{
			axis = 0;
			bUpdate = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(tWidth);
		if (ImGui::DragFloat(("##_vectorInputY" + FString::ToString(offset + (SizeType)v)).c_str(), &euler.y, 0.1f, 0, 0, "Y:%.3f", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
		{
			axis = 1;
			bUpdate = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(tWidth);
		if (ImGui::DragFloat(("##_vectorInputZ" + FString::ToString(offset + (SizeType)v)).c_str(), &euler.z, 0.1f, 0, 0, "Z:%.3f", bReadOnly ? ImGuiSliderFlags_ReadOnly : 0))
		{
			axis = 2;
			bUpdate = true;
		}

		if (bUpdate)
		{
			FVector deltaAngle = euler - prev;

			if (axis == 1)
				*v = FQuaternion::EulerAngles(deltaAngle.Radians()) * (*v);
			else
				*v *= FQuaternion::EulerAngles(deltaAngle.Radians());
		}
		r = bUpdate;
	}

	return r;
}
