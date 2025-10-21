
#include "ArrayEdit.h"
#include "Object/Object.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_thorium.h"

CArrayEdit::CArrayEdit(int numObjects, CObject** object, void** data, const FProperty* property) : IPropertyEditor(numObjects, object, data, property)
{
	arrData = (FArrayHelper*)property->typeHelper;
	
	arrType = new FProperty();
	arrType->type = arrData->objType;
	arrType->size = arrData->objSize;
	arrType->offset = 0;
	arrType->typeName = property->typeName;
}

CArrayEdit::~CArrayEdit()
{
	delete arrType;
}

void CArrayEdit::Render()
{
	SizeType size = arrData->Size(objects[0]);
	if (editors.Size() != size)
	{
		editors.Clear();

		for (SizeType i = 0; i < size; i++)
		{
			
		}
	}

}
