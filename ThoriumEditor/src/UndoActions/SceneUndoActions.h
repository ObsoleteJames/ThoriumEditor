#pragma once

#include "Game/World.h"
#include "Game/Entity.h"

#include <QUndoCommand>

class CmdRenameEntity : public QUndoCommand
{
public:
	CmdRenameEntity(CEntity* ent, const FString& prevName);

	void undo() final;
	void redo() final;

private:
	CWorld* world;
	SizeType entId;
	FString prevName;
	FString newName;
};

class CmdReparentComponent : public QUndoCommand
{
public:
	CmdReparentComponent(CSceneComponent* comp, CSceneComponent* prevParent);

	void undo() final;
	void redo() final;

private:
	CWorld* world;
	SizeType entId;
	SizeType compId;

	SizeType prevEntId = 0;
	SizeType prevCompId = 0;

	SizeType newEntId = 0;
	SizeType newCompId = 0;
};

class CmdDeleteEntity : public QUndoCommand
{
public:
	CmdDeleteEntity(CEntity* ent);

	void undo() final;
	void redo() final;

private:
	CWorld* world;
	FClass* type;
	FString entName;
	SizeType entId;

	FMemStream data;

	struct compData
	{
		FClass* type;
		SizeType id;
		FString name;
		bool bUserCreated;
	};

	TArray<compData> components;
};
