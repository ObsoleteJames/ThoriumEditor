
#include "PropertyEditor.h"

#include "Object/Class.h"
#include "Object/Object.h"
#include "Misc/Script.h"

#include "IntEdit.h"
#include "FloatEdit.h"
#include "BoolEdit.h"
#include "StringEdit.h"
#include "StructEdit.h"
#include "EnumEdit.h"
#include "ArrayEdit.h"
#include "ObjectPtrEdit.h"
#include "ClassPtrEdit.h"
#include "ColorEdit.h"
#include "VectorEdit.h"

#include "Console.h"

IPropertyEditor* IPropertyEditor::GetEditor(int numObjects, CObject** object, void** data, const FProperty* property)
{
	switch (property->type)
	{
	case EVT_STRUCT:
	{
		if (property->meta && property->meta->HasFlag("UIType"))
		{
			FString uiType = property->meta->FlagValue("UIType");
			if (uiType == "Color")
			{
				if (property->typeName == "FVector")
					return new CColor3Edit(numObjects, object, data, property);
				if (property->typeName == "FColor")
					return new CColor4Edit(numObjects, object, data, property);
			}
		}

		if (property->typeName == "FVector")
			return new CVectorEdit(numObjects, object, data, property);

		return new CStructEdit(numObjects, object, data, property);
	}
	case EVT_STRING:
		return new CStringEdit(numObjects, object, data, property);
	case EVT_ENUM:
		return new CEnumEdit(numObjects, object, data, property);
	case EVT_ARRAY:
		return nullptr;
	case EVT_OBJECT_PTR:
		return new CObjectPtrEdit(numObjects, object, data, property);
	case EVT_FLOAT:
		return new CFloatEdit(numObjects, object, data, property);
	case EVT_DOUBLE:
		return new CFloatEdit(numObjects, object, data, property);
	case EVT_INT:
		return new CIntEdit(numObjects, object, data, property);
	case EVT_UINT:
		return new CIntEdit(numObjects, object, data, property);
	case EVT_BOOL:
		return new CBoolEdit(numObjects, object, data, property);
	}

	return nullptr;
}

IPropertyEditor::IPropertyEditor(int numObjects, CObject** _object, void** data, const FProperty* p) : property((FProperty*)p)
{
	owners.Reserve(numObjects);
	objects.Reserve(numObjects);

	for (int i = 0; i < numObjects; i++)
	{
		owners.Add(_object[i]);
		objects.Add((void*)((SizeType)(data[i]) + p->offset));
	}

	type = _object[0]->GetClass();

	if (type->HasTag("VisibleCondition"))
		bHasVisibleCondition = true;
	if (type->HasTag("EditCondition"))
		bHasEditCondition = true;
}

void IPropertyEditor::Validate()
{
	if (owners.Size() == 0 || !owners[0])
		return;

	if (property->meta && property->meta->HasFlag("ValidateFunc"))
	{
		FString funcName = property->meta->FlagValue("ValidateFunc");
		const FFunction* func = owners[0]->GetClass()->GetFunction(funcName);

		if (!func)
			return;

		if (func->numArguments > 0)
		{
			CONSOLE_LogError("IPropertyEditor", "Variable validate function cannnot contain arguments! (func: " + funcName + ", var: " + property->name + ")");
			return;
		}

		FStack stack(8);
		for (auto& owner : owners)
			func->execFunc(owner, stack);
	}
}

bool IPropertyEditor::IsVisible()
{
	if (!bHasVisibleCondition)
		return true;

	return false;
}

bool IPropertyEditor::CanEdit()
{
	if (!bHasEditCondition)
		return true;

	return false;
}
