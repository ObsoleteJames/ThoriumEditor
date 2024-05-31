
#include "Layer.h"
#include "EditorEngine.h"
#include "EditorMenu.h"

CLayerType::CLayerType()
{
	auto* layers = (TArray<CLayerType*>*)&CLayer::GetLayerTypes();
	layers->Add(this);
}

CLayerType::~CLayerType()
{
	auto* layers = (TArray<CLayerType*>*) & CLayer::GetLayerTypes();
	layers->Erase(layers->Find(this));
}

CLayer* CLayerType::Instantiate()
{
	THORIUM_ASSERT(funcInstantiate != nullptr, "Attempted to instantiate CLayer but type has no way to be instantiated.");

	if (!bAllowMultiple && instances.Size() > 0)
		return instances.last();

	CLayer* l = funcInstantiate();
	l->type = this;
	instances.Add(l);
	return l;
}

CLayer::~CLayer()
{
	if (type)
		type->instances.Erase(type->instances.Find(this));
}

const TArray<CLayerType*>& CLayer::GetLayerTypes()
{
	static TArray<CLayerType*> layerTypes;
	return layerTypes;
}

CLayerType* CLayer::GetLayerType(const FString& name)
{
	auto& layers = GetLayerTypes();
	for (auto* l : layers)
	{
		if (l->Name() == name)
			return l;
	}
	return nullptr;
}

void CLayer::Init()
{
	auto& layers = GetLayerTypes();
	for (auto* l : layers)
	{
		if (!l->AllowMultiple())
		{
			auto* layer = l->Instantiate();
			layer->bEnabled = l->bEnabledByDefault;

			if (l->menuShortcut.IsEmpty())
				continue;

			FString menuName = l->menuShortcut;
			FString menuPath = l->menuShortcut;

			if (auto i = menuName.FindLastOf("/\\"); i != -1)
			{
				menuName.Erase(menuName.begin(), menuName.begin() + i + 1);
				menuPath.Erase(menuPath.begin() + i, menuPath.end());
			}

			CEditorMenu* menu = new CEditorMenu(menuName, l->menuCategory, FString());
			menu->OnClicked = [=]() { 
				layer->bEnabled = menu->bChecked;
			};
			gEditorEngine()->RegisterMenu(menu, menuPath);
			layer->menu = menu;
			menu->bChecked = layer->bEnabled;
		}
		else if (!l->menuShortcut.IsEmpty())
		{
			FString menuName = l->menuShortcut;
			FString menuPath = l->menuShortcut;

			if (auto i = menuName.FindLastOf("/\\"); i != -1)
			{
				menuName.Erase(menuName.begin(), menuName.begin() + i + 1);
				menuPath.Erase(menuPath.begin() + i, menuPath.end());
			}

			CEditorMenu* menu = new CEditorMenu(menuName, l->menuCategory, FString(), false);
			menu->OnClicked = [=]() {
				l->Instantiate();
			};
			gEditorEngine()->RegisterMenu(menu, menuPath);
		}
	}
}

void CLayer::LoadConfig(FKeyValue& cfg)
{
	auto& layers = GetLayerTypes();
	for (auto* l : layers)
	{
		if (!l->AllowMultiple())
		{
			auto* layer = l->GetInstance();
			if (!layer)
				continue;

			layer->bEnabled = cfg.GetValue("view_" + l->Name())->AsBool(l->bEnabledByDefault);
			if (layer->menu)
				layer->menu->bChecked = layer->bEnabled;
		}
	}
}

void CLayer::SaveConfig(FKeyValue& cfg)
{
	auto& layers = GetLayerTypes();
	for (auto* l : layers)
	{
		if (!l->AllowMultiple())
		{
			auto* layer = l->GetInstance();
			if (!layer)
				continue;

			cfg.SetValue("view_" + l->Name(), FString::ToString((int)layer->bEnabled));
		}
	}
}
