#pragma once

#include <Util/Core.h>
#include "EditorCore.h"

class SDK_API CEditorMenu
{
	friend class CEditorEngine;

public:
	CEditorMenu(const FString& name, bool toggle = true);
	CEditorMenu(const FString& name, const FString& shortcut, bool toggle = true);
	CEditorMenu(const FString& name, const FString& category, const FString& shortcut, bool toggle = true);
	~CEditorMenu();

	inline const FString& Name() const { return name; }
	inline const FString& Shortcut() const { return shortcut; }
	inline const FString& Category() const { return category; }

	inline CEditorMenu* Parent() const { return parent; }
	void SetParent(CEditorMenu* p);

	inline bool IsEnabled() const { return bEnabled; }
	inline void SetEnabled(bool b = true) { bEnabled = b; }

	void Render();

public:
	//void (*OnEnabled)() = nullptr;
	//void (*OnDisabled)() = nullptr;
	//void (*OnClicked)() = nullptr;

	std::function<void()> OnEnabled;
	std::function<void()> OnDisabled;
	std::function<void()> OnClicked;

	bool bHidden = false;
	bool bChecked = false;

private:
	// Sorts children by category, for rendering.
	void SortChildren();

	void RenderChildren();

private:
	CEditorMenu* parent;
	TArray<CEditorMenu*> children;

	FString name;
	FString shortcut;
	FString category;

	bool bToggle;
	bool bEnabled = true;
};
