
#include "ObjectDebugger.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Imgui/imgui_thorium.h"
#include "EditorWidgets.h"
#include "EditorEngine.h"
#include "EditorMenu.h"

#include <thread>
#include <chrono>

REGISTER_EDITOR_LAYER(CObjectDebugger, "Debug/Object Debugger", nullptr, false, false)

FString ToStringHex(SizeType i)
{
	FString r;
	bool bNegative = false;

	if (i < 0)
	{
		bNegative = true;
		i *= -1;
	}

	const char chars[] = {
		'0',
		'1',
		'2',
		'3',
		'4',
		'5',
		'6',
		'7',
		'8',
		'9',
		'A',
		'B',
		'C',
		'D',
		'E',
		'F'
	};

	do
	{
		int v = i % 16;
		r += chars[v];
		if (v < 10)
			r += '0';
		i /= 16;
	} while (i != 0);

	if (bNegative)
		r += '-';
	r.Reverse();
	return r;
}

struct FObjectUserInfo
{
	TObjectPtr<CObject> object;
	const FProperty* property;
};

struct FSearchUsersData
{
	CObject* target;
	TAtomic<float> progress = 0.f;
	TAtomic<bool> bRunning = false;
	TAtomic<bool> bDone = false;
	TAtomic<int> found = 0; // the amount of users found.

	TArray<FObjectUserInfo> results;
} searchUsersData;

static std::thread _suThread;

void threadObjSearchUsers()
{
	searchUsersData.bDone = false;
	searchUsersData.bRunning = true;
	searchUsersData.progress = 0.f;
	searchUsersData.found = 0;
	searchUsersData.results.Clear();

	auto& objs = CObjectManager::GetAllObjects();
	int searchCount = 0;
	for (auto& it : objs)
	{
		if (!searchUsersData.bRunning)
			break;

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(1ms);

		CObject* obj = it.second;
		FClass* c = it.second->GetClass();

		for (const FProperty* p = c->GetPropertyList(); p != nullptr; p = p->next)
		{
			if (p->type > EVT_OBJECT_PTR)
				continue;

			if (p->type == EVT_OBJECT_PTR)
			{
				TObjectPtr<CObject>* ptr = (TObjectPtr<CObject>*)((SizeType)obj + p->offset);
				if (*ptr == searchUsersData.target)
				{
					searchUsersData.found++;
					searchUsersData.results.Add({ obj, p });
				}
			}

			if (p->type == EVT_ARRAY && ((FArrayHelper*)p->typeHelper)->objType == EVT_OBJECT_PTR)
			{
				TArray<TObjectPtr<CObject>>* arr = (TArray<TObjectPtr<CObject>>*)((SizeType)obj + p->offset);
				for (auto& _ptr : *arr)
				{
					if (_ptr == searchUsersData.target)
					{
						searchUsersData.found++;
						searchUsersData.results.Add({ obj, p });
					}
				}
			}

			if (p->type == EVT_STRUCT)
			{
				FStruct* _struct = CModuleManager::FindStruct(p->typeName);
				if (!_struct)
					continue;

				for (const FProperty* _p = _struct->GetPropertyList(); _p != nullptr; _p = _p->next)
				{
					if (_p->type > EVT_OBJECT_PTR)
						continue;

					if (_p->type == EVT_OBJECT_PTR)
					{
						TObjectPtr<CObject>* ptr = (TObjectPtr<CObject>*)((SizeType)obj + _p->offset + p->offset);
						if (*ptr == searchUsersData.target)
						{
							searchUsersData.found++;
							searchUsersData.results.Add({ obj, _p });
						}
					}

					if (_p->type == EVT_ARRAY && ((FArrayHelper*)_p->typeHelper)->objType == EVT_OBJECT_PTR)
					{
						TArray<TObjectPtr<CObject>>* arr = (TArray<TObjectPtr<CObject>>*)((SizeType)obj + _p->offset + p->offset);
						for (auto& _ptr : *arr)
						{
							if (_ptr == searchUsersData.target)
							{
								searchUsersData.found++;
								searchUsersData.results.Add({ obj, _p });
							}
						}
					}
				}
			}
		}

		searchCount++;
		searchUsersData.progress = (float)searchCount / (float)objs.size();
	}

	if (searchCount == objs.size())
		searchUsersData.bDone = true;

	searchUsersData.bRunning = false;
}

void CObjectDebugger::OnUIRender()
{
	if (ImGui::Begin("Object Debugger", &bEnabled))
	{
		static bool bShowAddress = false;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
		if (ImGui::BeginChild("##_OD_objectsList", ImVec2(420, 0)))
		{
			if (ImGui::BeginTabBar("tabsObjects"))
			{
				if (ImGui::BeginTabItem("Object List"))
				{
					static FString filter;
					ImGui::InputText("Search", &filter);

					ImGui::Checkbox("Show Address", &bShowAddress);

					if (ImGui::BeginTable("tableObjectsLists", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
					{
						ImGui::TableSetupColumn("Name");
						ImGui::TableSetupColumn("Type");
						ImGui::TableHeadersRow();

						auto& objs = CObjectManager::GetAllObjects();

						for (auto obj : objs)
						{
							bool bSelected = obj.second == selected;
							FString objName = obj.second->Name();
							if (objName.IsEmpty())
							{
								//objName = "Object";
								objName = obj.second->GetClass()->GetName();
							}

							if (objName.Find(filter) == -1)
								continue;

							ImGui::TableNextRow();
							ImGui::TableNextColumn();

							if (ImGui::Selectable((objName + (bShowAddress ? " : 0x" : "##") + ToStringHex((SizeType)obj.second)).c_str(), bSelected, ImGuiSelectableFlags_SpanAllColumns))
							{
								selected = obj.second;
							}

							ImGui::TableNextColumn();
							ImGui::Text(obj.second->GetClass()->GetInternalName().c_str());
						}

						ImGui::EndTable();
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Class List"))
				{
					TMap<FClass*, int> classCount;

					static FString filter;
					ImGui::InputText("Search", &filter);

					auto& objs = CObjectManager::GetAllObjects();
					for (auto obj : objs)
						if (filter.IsEmpty() || obj.second->GetClass()->GetInternalName().ToLowerCase().Find(filter.ToLowerCase()) != -1)
							classCount[obj.second->GetClass()] += 1;
					
					if (ImGui::BeginTable("tableClassList", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
					{
						ImGui::TableSetupColumn("Class");
						ImGui::TableSetupColumn("Count");
						ImGui::TableHeadersRow();

						for (auto& c : classCount)
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();

							ImGui::Text(c.first->GetInternalName().c_str());

							ImGui::TableNextColumn();

							ImGui::Text("%d", c.second);
						}

						ImGui::EndTable();
					}


					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}

		ImGui::PopStyleColor();
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
		if (ImGui::BeginChild("##_OD_objectData"))
		{
			ImVec2 region = ImGui::GetContentRegionAvail();
			ImVec2 cursor = ImGui::GetCursorScreenPos();

			if (selected)
				ImGui::RenderFrame(cursor, cursor + region, ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.1f, 0.1f, 1.f)), false);

			if (selected && ImGui::BeginTable("tableObjectData", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
			{
				if (ImGui::TableTreeHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Name");
					ImGui::TableNextColumn();
					ImGui::InputText("##_objNameInput", (FString*)&selected->Name(), ImGuiInputTextFlags_ReadOnly);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Type");
					ImGui::TableNextColumn();
					ImGui::Text(selected->GetClass()->GetName().c_str());

					SizeType objId = selected->Id();
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("ID");
					ImGui::TableNextColumn();
					ImGui::Text(FString::ToString(objId).c_str());

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Owner");
					ImGui::TableNextColumn();
					if (ImGui::Button(selected->GetOwner() ? selected->GetOwner()->Name().c_str() : "Null") && selected->GetOwner())
						selected = selected->GetOwner();

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Users");
					ImGui::TableNextColumn();
					ImGui::Text(FString::ToString(selected->GetUserCount()).c_str());
					ImGui::SameLine();
					if (ImGui::Button("Search references") && !searchUsersData.bRunning)
					{
						searchUsersData.target = selected;
						searchUsersData.bRunning = true;
						_suThread = std::thread(threadObjSearchUsers);
						ImGui::OpenPopup("SearchObjectUsers");
					}

					if (ImGui::TableTreeHeader("Children", 0, true))
					{
						auto& children = selected->GetChildren();
						for (int i = 0; i < children.Size(); i++)
						{
							if (!children[i].IsValid())
								continue;

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text(children[i]->Name().c_str());
							ImGui::TableNextColumn();
							if (ImGui::Button(("Select##_btnSelectChild" + FString::ToString((SizeType) & *children[i])).c_str()))
								selected = children[i];
						}

						ImGui::TreePop();
					}

					ImGui::TreePop();
				}

				if (ImGui::TableTreeHeader("Properties"))
				{


					ImGui::TreePop();
				}

				if (ImGui::TableTreeHeader("Functions"))
				{


					ImGui::TreePop();
				}

				ImGui::EndTable();
			}
			else
				ImGui::Text("no object selected");

		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}
	ImGui::End();
	Menu()->bChecked = bEnabled;

	if (searchUsersData.bRunning)
		ImGui::OpenPopup("SearchObjectUsers");

	if (ImGui::BeginPopupModal("SearchObjectUsers"))
	{
		if (!searchUsersData.bDone)
		{
			ImGui::ProgressBar(searchUsersData.progress, ImVec2(0, 0), !searchUsersData.bRunning ? "Cancelled" : nullptr);

			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				searchUsersData.bRunning = false;
			}
			if (!searchUsersData.bRunning && _suThread.joinable())
			{
				_suThread.join();
				ImGui::CloseCurrentPopup();
			}

			ImGui::Text(("Found: " + FString::ToString((int)searchUsersData.found)).c_str());
		}
		else
		{
			ImGui::Text(("Found: " + FString::ToString((int)searchUsersData.found)).c_str());

			if (ImGui::BeginTable("searchObjectUsersResultsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
			{
				ImGui::TableSetupColumn("Object");
				ImGui::TableSetupColumn("Property");
				ImGui::TableHeadersRow();

				for (auto& obj : searchUsersData.results)
				{
					FString objName = obj.object->Name();
					if (objName.IsEmpty())
					{
						//objName = "Object";
						objName = obj.object->GetClass()->GetName();
					}

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text(objName.c_str());
					ImGui::TableNextColumn();
					ImGui::Text(obj.property->name.c_str());
				}

				ImGui::EndTable();
			}

			if (ImGui::Button("Close"))
			{
				_suThread.join();
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}
}
