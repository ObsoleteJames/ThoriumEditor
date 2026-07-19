#pragma once

#include "Editor.h"
#include "Object/Variant.h"

class EDITOR_API CEditorVar
{
public:
	CEditorVar(const FString& name, const FString& group, const FVariant& defaultValue, bool bShowInSettings = false);
	~CEditorVar();

	inline const FString& GetName() const { return name; }
	inline const FString& GetGroup() const { return group; }
	inline const FVariant& GetValue() const { return value; }

	inline void SetValue(const FVariant& newValue) { value = newValue; }

	inline bool ShowInSettings() const { return bShowInSettings; }

	void Revert();

	static void Save();
	static void Load();
	static const TArray<CEditorVar*>& GetVariables();

private:
	FString name;
	FString group;

	FVariant value;
	FVariant defaultValue;

	bool bShowInSettings = false;
};
