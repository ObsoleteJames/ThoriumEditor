#pragma once

#include <Util/Core.h>
#include <Util/Pointer.h>
#include "Widgets/PropertyEditor.h"
#include "Object/PropertyHandler.h"

struct FProperty;
class QCheckBox;
class QPushButton;

class CBoolProperty : public IBasePropertyEditor
{
	Q_OBJECT

public:
	CBoolProperty(bool* value, const FProperty* property, QWidget* parent = nullptr);
	
	void Update();

	void SetValue(bool* value) { handler->SetValue(value); Update(); }

private:
	QCheckBox* editor;

	TUniquePtr<IPropertyHandler> handler;
	QPushButton* revertBtn;
};
