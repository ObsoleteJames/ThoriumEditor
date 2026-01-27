
#include "EnumProperty.h"
#include "Object/Class.h"
#include "Module.h"

#include <QLabel>
#include <QComboBox>
#include <QBoxLayout>
#include <QVariant>

CEnumProperty::CEnumProperty(void* ptr, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value(ptr)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());

	editor = new QComboBox(this);

	FEnum* type = CModuleManager::FindEnum(property->typeName);
	THORIUM_ASSERT(type, "Failed to find enum type.");

	options = type->GetValues();
	for (auto& p : options)
		editor->addItem(p.Key.c_str(), QVariant(p.Value));

	QLabel* label = new QLabel(property->name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	Update();
	connect(editor, SIGNAL(currentIndexChanged(int)), this, SLOT(valueChanged(int)));
}

CEnumProperty::CEnumProperty(const FString& name, void* ptr, FEnum* type, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value(ptr)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());

	editor = new QComboBox(this);

	byteSize = type->Size();
	options = type->GetValues();
	for (auto& p : options)
		editor->addItem(p.Key.c_str(), QVariant(p.Value));

	QLabel* label = new QLabel(name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	Update();
	connect(editor, SIGNAL(currentIndexChanged(int)), this, SLOT(valueChanged(int)));
}

CEnumProperty::CEnumProperty(const FString& name, int* ptr, const TArray<TPair<FString, int64>>& o, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value(ptr), byteSize(4)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());

	editor = new QComboBox(this);

	options = o;
	for (auto& p : options)
		editor->addItem(p.Key.c_str(), QVariant(p.Value));

	QLabel* label = new QLabel(name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	Update();
	connect(editor, SIGNAL(currentIndexChanged(int)), this, SLOT(valueChanged(int)));
}

void CEnumProperty::Update()
{
	int64 curValue = 0;
	switch (byteSize)
	{
	case 1:
		curValue = *(int8*)value;
		break;
	case 2:
		curValue = *(int16*)value;
		break;
	case 4:
		curValue = *(int32*)value;
		break;
	case 8:
		curValue = *(int64*)value;
		break;
	}
	if (editor->currentData().toLongLong() == curValue)
		return;
	
	int index = 0;
	for (auto i = 0ull; i < options.Size(); i++)
	{
		if (options[i].Value == curValue)
		{
			index = i;
			break;
		}
	}

	editor->setCurrentIndex(index);
}

void CEnumProperty::valueChanged(int index)
{
	int64 newValue = editor->itemData(index).toLongLong();

	switch (byteSize)
	{
	case 1:
		*(int8*)value = newValue;
		break;
	case 2:
		*(int16*)value = newValue;
		break;
	case 4:
		*(int32*)value = newValue;
		break;
	case 8:
		*(int64*)value = newValue;
		break;
	}

	emit(OnValueChanged());
}

void CEnumProperty::ResetOptions()
{
	editor->clear();
	for (auto& p : options)
		editor->addItem(p.Key.c_str(), QVariant(p.Value));
}
