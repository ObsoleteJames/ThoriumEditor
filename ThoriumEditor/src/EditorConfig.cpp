
#include "EditorConfig.h"
#include "System.h"
#include <Util/KeyValue.h>

static TArray<CEditorVar*>& _EditorVariables()
{
	static TArray<CEditorVar*> vars;
	return vars;
}

CEditorVar::CEditorVar(const FString& n, const FString& g, const FVariant& v, bool bS) : name(n), group(g), value(v), defaultValue(v), bShowInSettings(bS)
{
	_EditorVariables().Add(this);
}

CEditorVar::~CEditorVar()
{
	if (auto it = _EditorVariables().Find(this); it != _EditorVariables().end())
		_EditorVariables().Erase(it);
}

void CEditorVar::Revert()
{
	value = defaultValue;
}

void CEditorVar::Save()
{
	FKeyValue kv(SSystem::GetDataPath() + "/ThoriumEngine/EditorConfig/Editor.cfg");

	for (auto& var : _EditorVariables())
	{
		FString name = var->name;
		name.ReplaceAll(' ', '_');
		name.ReplaceAll('\t', '_');

		kv.SetValue(name, var->value.ToString());
	}

	kv.Save();
}

void CEditorVar::Load()
{
	FKeyValue kv(SSystem::GetDataPath() + "/ThoriumEngine/EditorConfig/Editor.cfg");
	if (!kv.IsOpen())
		return;
	
	for (auto& var : _EditorVariables())
	{
		FString name = var->name;
		name.ReplaceAll(' ', '_');
		name.ReplaceAll('\t', '_');

		auto* v = kv.GetValue(name, false);
		if (v)
			var->value = FVariant::FromString(v->Value);
		else
			var->value = var->defaultValue;
	}
}

const TArray<CEditorVar*>& CEditorVar::GetVariables()
{
	return _EditorVariables();
}
