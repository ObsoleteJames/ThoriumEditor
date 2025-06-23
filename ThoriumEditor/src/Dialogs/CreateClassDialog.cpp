
#define IMGUI_DEFINE_MATH_OPERATORS
#include "CreateClassDialog.h"

#include "ImGui/imgui_thorium.h"

struct FStructTreeNode
{
	FStruct* Struct = nullptr;
	TArray<FStructTreeNode> children;

	void Render(FClass*& selected)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (children.Size() == 0)
			flags |= ImGuiTreeNodeFlags_Leaf;

		if (selected == Struct || selected == Struct)
			flags |= ImGuiTreeNodeFlags_Selected;

		bool bOpen = true;
		if (Struct)
		{
			bOpen = ImGui::TreeNodeEx(Struct->GetName().c_str(), flags);

			if (ImGui::IsItemClicked())
			{
				selected = (FClass*)Struct;
			}
		}

		if (bOpen)
		{
			for (auto& c : children)
				c.Render(selected);

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

CCreateClassDialog::CCreateClassDialog() : CDialogWnd("Create C++ Class", EDialogFlags_NoResize)
{
	windowSize = { 300, 90 };
}

void CCreateClassDialog::Render()
{
	if (ImGui::Begin("Dialog", 0, DIALOG_WND_FLAGS))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			Finish(0);

		ImVec2 size = ImGui::GetContentRegionAvail();
		ImVec2 cursor = ImGui::GetCursorPos();

		
	}
	ImGui::End();
}
