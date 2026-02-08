
#include "EnumProperty.h"
#include "Object/Class.h"
#include "Module.h"

#include <QLabel>
#include <QComboBox>
#include <QBoxLayout>
#include <QVariant>
#include <QUndoCommand>

CEnumProperty::CEnumProperty(void* ptr, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value(ptr)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());
	undoName = property->name;

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
	undoName = name;

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
	undoName = name;

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
	class Undo : public QUndoCommand
	{
	public:
		Undo(const QString& name, void* ptr, uint8 byteSize, int64 oldv, int64 newv)
			: QUndoCommand(name), ptr(ptr), byteSize(byteSize), oldValue(oldv), newValue(newv)
		{
		}

		int id() const override
		{
			return 1006;
		}

		bool mergeWith(const QUndoCommand* other) override
		{
			auto* cmd = static_cast<const Undo*>(other);
			if (!cmd || cmd->ptr != ptr || cmd->byteSize != byteSize)
				return false;
			newValue = cmd->newValue;
			return true;
		}

		void undo() override
		{
			switch (byteSize)
			{
			case 1:
				*(int8*)ptr = (int8)oldValue;
				break;
			case 2:
				*(int16*)ptr = (int16)oldValue;
				break;
			case 4:
				*(int32*)ptr = (int32)oldValue;
				break;
			case 8:
				*(int64*)ptr = (int64)oldValue;
				break;
			}
		}

		void redo() override
		{
			switch (byteSize)
			{
			case 1:
				*(int8*)ptr = (int8)newValue;
				break;
			case 2:
				*(int16*)ptr = (int16)newValue;
				break;
			case 4:
				*(int32*)ptr = (int32)newValue;
				break;
			case 8:
				*(int64*)ptr = (int64)newValue;
				break;
			}
		}

		void* ptr;
		uint8 byteSize;
		int64 oldValue;
		int64 newValue;
	};

	int64 oldValue = 0;
	switch (byteSize)
	{
	case 1:
		oldValue = *(int8*)value;
		break;
	case 2:
		oldValue = *(int16*)value;
		break;
	case 4:
		oldValue = *(int32*)value;
		break;
	case 8:
		oldValue = *(int64*)value;
		break;
	}

	int64 newValue = editor->itemData(index).toLongLong();
	if (oldValue == newValue)
		return;

	curUndoCmd = new Undo((undoName + " Value Edited").c_str(), value, byteSize, oldValue, newValue);

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
