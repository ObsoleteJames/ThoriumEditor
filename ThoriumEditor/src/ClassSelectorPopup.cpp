
#include "ClassSelectorPopup.h"
#include "Object/Object.h"

#include "Game/Events.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/ImGui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"

static bool bInit = false;
static bool bSelected = false;
static bool bClose = false;
static int bMode = 0;

static FStruct* filter = nullptr;

static FClass* curClass = nullptr;
static FStruct* curStruct = nullptr;

static FString search;

static SizeType curId;

struct FStructTreeNode
{
	FStruct* Struct = nullptr;
	TArray<FStructTreeNode> children;

	void Render()
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (children.Size() == 0)
			flags |= ImGuiTreeNodeFlags_Leaf;

		if (curClass == Struct || curStruct == Struct)
			flags |= ImGuiTreeNodeFlags_Selected;

		bool bOpen = true;
		if (Struct)
		{
			bOpen = ImGui::TreeNodeEx(Struct->GetName().c_str(), flags);

			if (ImGui::IsItemClicked())
			{
				if (bMode == 1)
					curClass = (FClass*)Struct;
				else
					curStruct = Struct;
			}
		}

		if (bOpen)
		{
			for (auto& c : children)
				c.Render();

			if (Struct)
				ImGui::TreePop();
		}
	}
};

struct FStructTree
{
public:
	FStructTree() = default;

	void GenerateClass(FClass* base = nullptr)
	{
		root.Struct = nullptr;
		root.children.Clear();

		if (!base)
			base = CObject::StaticClass();

		if (base->IsClass())
		{
			root.Struct = base;
			GenerateNode(&root);
		}
	}

	void GenerateStruct()
	{
		root.Struct = nullptr;
		root.children.Clear();

		const auto& modules = CModuleManager::GetModules();
		for (auto& m : modules)
		{
			for (auto& s : m->Structures)
			{
				root.children.Add({ s });
			}
		}
	}

	void GenerateNode(FStructTreeNode* node)
	{
		TArray<FClass*> children;
		CModuleManager::FindChildClasses((FClass*)node->Struct, children);

		for (auto& c : children)
		{
			node->children.Add({ c });
			GenerateNode(node->children.last());
		}
	}

	FStructTreeNode root;
} static structTree;

class CThClassSelector : public CObject
{
public:
	CThClassSelector()
	{
		name = "ThClassSelectorDialog";
		Events::OnRender.Bind(this, &CThClassSelector::Update);
	}

	void Update()
	{
		if (bMode == 1 && !ImGui::IsPopupOpen("Select Class##thSelectClassDialog"))
			ImGui::OpenPopup("Select Class##thSelectClassDialog");

		ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
		if (ImGui::BeginPopupModal("Select Class##thSelectClassDialog", nullptr, 0))
		{
			ImVec2 content = ImGui::GetContentRegionAvail();
			if (ImGui::BeginChild("structTreeArea", ImVec2(0, content.y - 32)))
			{
				structTree.root.Render();
			}
			ImGui::EndChild();

			ImGui::SetCursorPosX(content.x - 90);

			if (ImGui::Button("Cancel"))
			{
				curId = 0;
				bMode = 0;
				bSelected = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();

			if (!curClass)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
			}

			if ((ImGui::Button("Select")) && curClass)
				bSelected = true;

			if (!curClass)
				ImGui::PopStyleColor(3);

			if (bClose)
			{
				ImGui::CloseCurrentPopup();
				bClose = false;
			}
			ImGui::EndPopup();
		}
	}
};

static TObjectPtr<CThClassSelector> handler;

void ThClassSelector_Init()
{
	if (bInit)
		return;

	handler = new CThClassSelector();
	handler->Update();
	bInit = true;
}

bool ThoriumEditor::SelectClass(const FString& id, FClass* base /*= nullptr*/)
{
	if (!bInit)
		ThClassSelector_Init();

	if (curId != 0)
		return false;

	filter = base;
	curId = id.Hash();
	bMode = 1;
	curClass = nullptr;
	search.Clear();
	bSelected = false;

	structTree.GenerateClass(base);

	ImGui::OpenPopup("Select Class##thSelectClassDialog");
	return true;
}

bool ThoriumEditor::SelectStruct(const FString& id, FStruct* base /*= nullptr*/)
{
	if (!bInit)
		ThClassSelector_Init();

	return false;
}

bool ThoriumEditor::AcceptClass(const FString& id, FClass** outClass)
{
	if (curId != id.Hash())
		return false;

	if (bMode == 1 && bSelected)
	{
		*outClass = curClass;
		curId = 0;
		bSelected = false;
		bClose = true;
		bMode = 0;
		return true;
	}

	return false;
}

bool ThoriumEditor::AcceptStruct(const FString& id, FStruct** outStruct)
{
	if (curId != id.Hash())
		return false;

	if (bMode == 1 && bSelected)
	{
		*outStruct = curStruct;
		curId = 0;
		bSelected = false;
		bClose = true;
		bMode = 0;
		return true;
	}

	return false;
}

void ThoriumEditor::CancelClassSelectDialog()
{

}

