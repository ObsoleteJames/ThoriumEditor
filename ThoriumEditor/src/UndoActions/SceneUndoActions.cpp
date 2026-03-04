
#include "SceneUndoActions.h"

CmdRenameEntity::CmdRenameEntity(CEntity* ent, const FString& pn) : QUndoCommand("Rename Entity")
{
	world = ent->GetWorld();
	entId = ent->EntityId();
	prevName = pn;
	newName = ent->Name();
}

void CmdRenameEntity::undo()
{
	CEntity* ent = world->GetEntity(entId);
	ent->SetName(prevName);
}

void CmdRenameEntity::redo()
{
	CEntity* ent = world->GetEntity(entId);
	ent->SetName(newName);
}

CmdReparentComponent::CmdReparentComponent(CSceneComponent* comp, CSceneComponent* prevParent) : QUndoCommand("Parent Entity/Component")
{
	world = comp->GetWorld();
	entId = comp->GetEntity()->EntityId();
	compId = comp->ComponentId();

	if (prevParent)
	{
		prevEntId = prevParent->GetEntity()->EntityId();
		prevCompId = prevParent->ComponentId();
	}

	if (comp->GetParent())
	{
		newEntId = comp->GetParent()->GetEntity()->EntityId();
		newCompId = comp->GetParent()->ComponentId();
	}
}

void CmdReparentComponent::undo()
{
	CEntity* ent = world->GetEntity(entId);
	CSceneComponent* comp = ent->GetComponent<CSceneComponent>(compId);

	if (prevEntId != 0)
	{
		CEntity* prevEnt = world->GetEntity(prevEntId);
		CSceneComponent* prevComp = prevEnt->GetComponent<CSceneComponent>(prevCompId);

		comp->AttachTo(prevComp);
	}
	else
		comp->Detach();
}

void CmdReparentComponent::redo()
{
	CEntity* ent = world->GetEntity(entId);
	CSceneComponent* comp = ent->GetComponent<CSceneComponent>(compId);

	if (newEntId)
	{
		CEntity* prevEnt = world->GetEntity(newEntId);
		CSceneComponent* prevComp = prevEnt->GetComponent<CSceneComponent>(newCompId);

		comp->AttachTo(prevComp);
	}
	else
		comp->Detach();
}

CmdDeleteEntity::CmdDeleteEntity(CEntity* ent) : QUndoCommand("Delete Entity")
{
	world = ent->GetWorld();
	entId = ent->EntityId();
	type = ent->GetClass();
	entName = ent->Name();

	for (auto comp : ent->GetAllComponents())
		components.Add({ comp.second->GetClass(), comp.first, comp.second->Name(), comp.second->IsUserCreated() });

	ent->Serialize(data, FSerializeSettings());
}

void CmdDeleteEntity::undo()
{
	auto* ent = world->CreateEntity(type, entName);
	ent->SetEntityId(entId);

	for (auto& comp : components)
	{
		CEntityComponent* c = nullptr;
		if (!comp.bUserCreated)
		{
			c = ent->GetComponent(comp.type, comp.name);

			if (c)
				c->SetComponentId(comp.id);
		}
		if (!c)
			c = ent->AddComponent(comp.type, comp.id);
	}

	ent->Load(data);
}

void CmdDeleteEntity::redo()
{
	world->GetEntity(entId)->Delete();
}
