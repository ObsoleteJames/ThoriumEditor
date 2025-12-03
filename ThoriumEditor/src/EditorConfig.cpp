
#include "EditorConfig.h"
#include "System.h"
#include <Util/KeyValue.h>

static TArray<CEditorVar*>& _EditorVariables()
{
	static TArray<CEditorVar*> vars;
	return vars;
}

CEditorVar::CEditorVar(const FString& n, const FString& g, const FVariant& v) : name(n), group(g), value(v), defaultValue(v)
{
	_EditorVariables().Add(this);
}

CEditorVar::~CEditorVar()
{
	if (auto it = _EditorVariables().Find(this); it != _EditorVariables().end())
		_EditorVariables().Erase(it);
}

void CEditorVar::Save()
{
	FKeyValue kv(SSystem::GetDataPath() + "/ThoriumEngine/EditorConfig/Editor.cfg");

	for (auto& var : _EditorVariables())
		kv.SetValue(var->name, var->value.ToString());

	kv.Save();
}

void CEditorVar::Load()
{
	FKeyValue kv(SSystem::GetDataPath() + "/ThoriumEngine/EditorConfig/Editor.cfg");
	if (!kv.IsOpen())
		return;
	
	for (auto& var : _EditorVariables())
	{
		auto* v = kv.GetValue(var->name, false);
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
