#pragma once

#include "Game/World.h"
#include "Game/Entity.h"

#include <QUndoCommand>

class CmdTranslateEntity : public QUndoCommand
{
public:
	CmdTranslateEntity(CEntity* ent, const FTransform& deltaTransform);
	CmdTranslateEntity(const TArray<CEntity*> ents, const FTransform& deltaTransform);
	CmdTranslateEntity(const TArray<TObjectPtr<CEntity>> ents, const FTransform& deltaTransform);

	void undo() final;
	void redo() final;

	bool mergeWith(const QUndoCommand* other) final;

	int id() const final;

private:
	TArray<SizeType> ents;
	FTransform deltaTransform;

	bool bInit = false;
};
