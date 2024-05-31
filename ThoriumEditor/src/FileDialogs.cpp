
#include "EditorEngine.h"
#include "FileDialogs.h"
#include "Game/Events.h"
#include "Object/Object.h"

#include "AssetBrowserWidget.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/ImGui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"

static bool bInit = false;
static bool bSelected = false;
static bool bClose = false;
static int bMode = 0;

static FString saveFileName;

static CAssetBrowserWidget* browser;
static SizeType curId = 0;

// fucking terrible hack to make this work
class CThFileDialog : public CObject
{
public:
	CThFileDialog()
	{
		name = "ThFileDialogHandler";
		browser = new CAssetBrowserWidget();
		browser->bAllowFileEdit = false;
		Events::OnRender.Bind(this, &CThFileDialog::Update);
	}

	void Update()
	{
		ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
		if (ImGui::BeginPopupModal("Open File##thFileDialogOpenFile", nullptr, 0))
		{
			ImVec2 size = ImGui::GetContentRegionAvail();

			browser->RenderUI(0, size.y - 32);

			ImGui::SetCursorPosX(size.x - 90);

			if (ImGui::Button("Cancel"))
			{
				curId = 0;
				bSelected = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();

			if (!browser->GetSelectedFile())
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
			}

			if ((ImGui::Button("Open") || browser->bDoubleClickedFile) && browser->GetSelectedFile())
				bSelected = true;

			if (!browser->GetSelectedFile())
				ImGui::PopStyleColor(3);

			if (bClose)
			{
				ImGui::CloseCurrentPopup();
				bClose = false;
			}

			ImGui::EndPopup();
		}

		ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
		if (ImGui::BeginPopupModal("Save File##thFileDialogSaveFile", nullptr, 0))
		{
			ImVec2 size = ImGui::GetContentRegionAvail();

			browser->RenderUI(0, size.y - 32);

			ImGui::InputText("Name", &saveFileName);
			ImGui::SameLine();

			ImGui::SetCursorPosX(size.x - 90);

			if (ImGui::Button("Cancel"))
			{
				curId = 0;
				bSelected = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();

			if (saveFileName.IsEmpty())
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
			}

			if (ImGui::Button("Save") && !saveFileName.IsEmpty())
				bSelected = true;

			if (saveFileName.IsEmpty())
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

static TObjectPtr<CThFileDialog> dialogHandler;

void ThFileDialog_Init()
{
	if (dialogHandler || bInit)
		return;

	dialogHandler = new CThFileDialog();
	// Call update so that the popups work.
	dialogHandler->Update();
	bInit = true;
}

bool ThoriumEditor::OpenFile(const FString& id, FAssetClass* type, const FString& dir /*= FString()*/)
{
	if (!bInit)
		ThFileDialog_Init();

	if (curId != 0)
		return false;

	if (!dir.IsEmpty())
	{
		FString m;
		FString d;
		browser->ExtractPath(dir, m, d);
		browser->SetDir(m, d);
	}
	else
		browser->SetDir(gEditorEngine()->ActiveGame().mod ? gEditorEngine()->ActiveGame().mod->Name() : "Engine", FString());

	browser->viewFilter = type;
	browser->bDoubleClickedFile = false;
	curId = id.Hash();
	bSelected = false;
	bMode = 1;

	ImGui::OpenPopup("Open File##thFileDialogOpenFile");
	return true;
}

bool ThoriumEditor::SaveFile(const FString& id, FAssetClass* type, const FString& dir /*= FString()*/)
{
	if (!bInit)
		ThFileDialog_Init();

	if (curId != 0)
		return false;

	if (!dir.IsEmpty())
	{
		FString m;
		FString d;
		browser->ExtractPath(dir, m, d);
		browser->SetDir(m, d);
	}
	else
		browser->SetDir(gEditorEngine()->ActiveGame().mod ? gEditorEngine()->ActiveGame().mod->Name() : "Engine", FString());

	browser->viewFilter = type;
	browser->bDoubleClickedFile = false;
	curId = id.Hash();
	bSelected = false;
	bMode = 2;
	saveFileName.Clear();

	ImGui::OpenPopup("Save File##thFileDialogSaveFile");
	return true;
}

bool ThoriumEditor::AcceptFile(const FString& id, FString* outFile, FString* outMod)
{
	if (curId != id.Hash())
		return false;

	if (bMode == 1 && bSelected && browser->GetSelectedFile())
	{
		*outFile = browser->GetSelectedFile()->Path();
		if (outMod)
			*outMod = browser->GetSelectedFile()->Mod()->Name();
		curId = 0;
		bSelected = false;
		bClose = true;
		bMode = 0;
	}
	if (bMode == 2 && bSelected)
	{
		*outFile = browser->dir + "/" + saveFileName;
		if (outMod)
			*outMod = browser->mod;
		curId = 0;
		bSelected = false;
		bClose = true;
		bMode = 0;
	}
	return true;
}

void ThoriumEditor::CancelFileDialog(const FString& id)
{
	if (curId == 0)
		return;

	bSelected = false;
	curId = 0;
	bMode = 0;
	bClose = true;
}
