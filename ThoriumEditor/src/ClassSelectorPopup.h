#pragma once

#include "EngineCore.h"

class FClass;
class FStruct;

namespace ThoriumEditor
{
	bool SelectClass(const FString& id, FClass* base = nullptr);
	bool SelectStruct(const FString& id, FStruct* base = nullptr);

	bool AcceptClass(const FString& id, FClass** outClass);
	bool AcceptStruct(const FString& id, FStruct** outStruct);

	void CancelClassSelectDialog();
}
