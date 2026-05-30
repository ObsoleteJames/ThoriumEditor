
#include "EditorPlugins.h"
#include "Console.h"

#include <Util/KeyValue.h>

static TArray<FEditorPlugin> plugins;

bool LoadPlugin(FEditorPlugin& plugin)
{
	if (plugin.lib)
		return true;

	for (auto& d : plugin.dependancies)
	{
		auto* dep = CEditorPlugins::GetPlugin(d);
		if (!dep)
		{
			CONSOLE_LogError("CEditorPlugins", "Unkown plugin dependancy '" + d + "', aborting loading '" + plugin.name + "'!");
			return false;
		}

		LoadPlugin(*dep);
	}

	plugin.lib = CModuleManager::LoadFLibrary(plugin.name, plugin.libPath);
	if (!plugin.lib)
		return false;

	return true;
}

void CEditorPlugins::Init()
{
	FKeyValue kv("plugins.cfg");
	if (!kv.IsOpen())
	{
		CONSOLE_LogInfo("CEditorPlugins", "plugins.cfg file not found, no plugins were loaded.");
		return;
	}

	for (auto* p : kv.GetCategories())
	{
		FEditorPlugin plugin{};
		plugin.name = p->GetName();

		plugin.bEnabled = p->GetValue("enabled")->AsBool();
		plugin.bInternal = p->GetValue("internal")->AsBool();

		plugin.libPath = *p->GetValue("lib");
		plugin.dependancies = *p->GetArray("dependancies", true);
		plugins.Add(plugin);
	}

	for (auto& p : plugins)
	{
		if (!p.bEnabled)
			continue;

		if (!LoadPlugin(p))
			CONSOLE_LogError("CEditorPlugins", "Failed to load plugin '" + p.name + "'!");
	}
}

void CEditorPlugins::Exit()
{
	for (auto& p : plugins)
	{
		if (p.lib)
			CModuleManager::UnloadLibrary(p.lib);

		p.lib = nullptr;
	}
}

FEditorPlugin* CEditorPlugins::GetPlugin(const FString& name)
{
	for (auto& p : plugins)
		if (p.name == name)
			return &p;
	return nullptr;
}
