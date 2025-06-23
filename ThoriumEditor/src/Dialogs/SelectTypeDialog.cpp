
#define IMGUI_DEFINE_MATH_OPERATORS
#include "SelectTypeDialog.h"

#include "ImGui/imgui_thorium.h"

struct FStructTreeNode
{
	FStruct* Struct = nullptr;
	TArray<FStructTreeNode> children;

	void Render(FClass*& selected)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
		if (children.Size() == 0)
			flags |= ImGuiTreeNodeFlags_Leaf;

		if (selected == Struct)
			flags |= ImGuiTreeNodeFlags_Selected;


		bool bAbstract = (Struct->IsClass() && ((FClass*)Struct)->Flags() & CTAG_ABSTRACT != 0);
		if (bAbstract)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.5f));

		bool bOpen = true;
		if (Struct)
		{
			bOpen = ImGui::TreeNodeEx(Struct->GetName().c_str(), flags);

			if (ImGui::IsItemClicked())
			{
				selected = (FClass*)Struct;
			}
		}

		if (bAbstract)
			ImGui::PopStyleColor();

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
};

CSelectClassDialog::CSelectClassDialog(const FString& t, FClass* f) : CDialogWnd(t), filter(f)
{
	windowSize = { 670, 430 };
	tree = new FStructTree();
	tree->GenerateClass(filter);
}

void CSelectClassDialog::Render()
{
	if (ImGui::Begin("ChoiceDialog", 0, DIALOG_WND_FLAGS))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			Finish(0);

		ImVec2 size = ImGui::GetContentRegionAvail();

		if (ImGui::BeginChild("##classTree", size - ImVec2(0, 32), true))
			tree->root.Render(selected);
		ImGui::EndChild();

		auto& style = ImGui::GetStyle();
		ImGui::SetCursorPosX(size.x - 128 - style.FramePadding.x);

		ImGui::BeginDisabled(!IsSelectionValid());
		if (ImGui::Button("Select", ImVec2(64, 24)))
			Finish(1);
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(64, 24)))
			Finish(0);
	}
	ImGui::End();
}

bool CSelectClassDialog::IsSelectionValid()
{
	if (selected == nullptr)
		return false;

	if (selected->IsClass() && ((FClass*)selected)->Flags() & CTAG_ABSTRACT != 0)
		return false;

	return true;
}

CSelectStructDialog::CSelectStructDialog(const FString& t, FStruct* f) : CDialogWnd(t), filter(f)
{
	windowSize = { 670, 430 };
	tree = new FStructTree();
}

void CSelectStructDialog::Render()
{
	if (ImGui::Begin("ChoiceDialog", 0, DIALOG_WND_FLAGS))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			Finish(0);

		ImVec2 size = ImGui::GetContentRegionAvail();



	}
	ImGui::End();
}

