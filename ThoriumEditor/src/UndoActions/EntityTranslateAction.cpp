
#include "EntityTranslateAction.h"

CmdTranslateEntity::CmdTranslateEntity(CEntity* ent, const FTransform& dt) : QUndoCommand("Translated Entity"), deltaTransform(dt)
{
	ents.Add(ent->EntityId());
}

CmdTranslateEntity::CmdTranslateEntity(const TArray<CEntity*> ents, const FTransform& dt) : QUndoCommand("Translated Entity"), deltaTransform(dt)
{
	for (auto& ent : ents)
		this->ents.Add(ent->EntityId());
}

CmdTranslateEntity::CmdTranslateEntity(const TArray<TObjectPtr<CEntity>> ents, const FTransform& dt) : QUndoCommand("Translated Entity"), deltaTransform(dt)
{
	for (auto& ent : ents)
		this->ents.Add(ent->EntityId());
}

void CmdTranslateEntity::undo()
{
	FTransform inverseDelta = deltaTransform.Inverse();
	for (SizeType entId : ents)
	{
		CEntity* ent = gWorld->GetEntity(entId);
		if (ent)
		{
			FTransform transform = inverseDelta + ent->RootComponent()->GetWorldTransform();
			ent->SetPosition(transform.position);
			ent->SetRotation(transform.rotation);
			ent->SetScale(transform.scale);
		}
	}
}

void CmdTranslateEntity::redo()
{
	if (!bInit)
	{
		bInit = true;
		return;
	}

	for (SizeType entId : ents)
	{
		CEntity* ent = gWorld->GetEntity(entId);
		if (ent)
		{
			FTransform transform = deltaTransform + ent->RootComponent()->GetWorldTransform();
			ent->SetPosition(transform.position);
			ent->SetRotation(transform.rotation);
			ent->SetScale(transform.scale);
		}
	}
}

bool CmdTranslateEntity::mergeWith(const QUndoCommand* other)
{
	const CmdTranslateEntity* otherCmd = (const CmdTranslateEntity*)other;
	if (otherCmd->ents.Size() != ents.Size())
		return false;
	for (int i = 0; i < ents.Size(); i++)
	{
		if (otherCmd->ents[i] != ents[i])
			return false;
	}

	deltaTransform = deltaTransform * otherCmd->deltaTransform;
	return true;
}

int CmdTranslateEntity::id() const
{
	return 1201;
}
