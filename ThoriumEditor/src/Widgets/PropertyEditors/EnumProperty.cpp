
#include "EnumProperty.h"
#include "Object/Class.h"
#include "Object/PropertyHandler.h"
#include "Module.h"
#include "Widgets/CollapsableWidget.h"

#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QBoxLayout>
#include <QVariant>
#include <QUndoCommand>
#include <QCheckBox>

class CEnumValueUndo : public QUndoCommand
{
public:
	CEnumValueUndo(const QString& name, void* ptr, uint8 byteSize, int64 oldv, int64 newv)
		: QUndoCommand(name), ptr(ptr), byteSize(byteSize), oldValue(oldv), newValue(newv)
	{
	}

	int id() const override
	{
		return 1006;
	}

	bool mergeWith(const QUndoCommand* other) override
	{
		auto* cmd = static_cast<const CEnumValueUndo*>(other);
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

CEnumProperty::CEnumProperty(void* ptr, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{
	this->property = property;
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());
	undoName = property->name;
	handler = property->GetHandler(ptr);
	value = handler->GetValue();

	FEnum* type = CModuleManager::FindEnum(property->typeName);
	THORIUM_ASSERT(type, "Failed to find enum type.");

	options = type->GetValues();
	byteSize = type->Size();

	bIsFlag = (type->Flags() & EEnumFlags::EnumFlag_IS_FLAG) != 0;
	if (bIsFlag)
		InitFlags(property->name);
	else
		InitEnum(property->name);
}

CEnumProperty::CEnumProperty(const FString& name, void* ptr, FEnum* type, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());
	undoName = name;
	value = ptr;

	byteSize = type->Size();
	options = type->GetValues();

	InitEnum(name);
}

CEnumProperty::CEnumProperty(const FString& name, int* ptr, const TArray<TPair<FString, int64>>& o, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), byteSize(4)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());
	undoName = name;
	value = ptr;

	options = o;

	InitEnum(name);
}

void CEnumProperty::Update()
{
	if (handler)
		value = handler->GetValue();

	if (!bIsFlag)
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
	else
	{
		for (int i = 0; i < options.Size(); i++)
		{
			//bool bChecked = *(int32*)value & options[i].Value;
			//flagEditors[i]->setChecked(bChecked);
		}
	}
}

void CEnumProperty::InitEnum(const FString& name)
{
	editor = new QComboBox(this);
	for (auto& p : options)
		editor->addItem(p.Key.c_str(), QVariant(p.Value));

	QLabel* label = new QLabel(name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	revertBtn = AddRevertBtn(handler);
	layout()->addWidget(revertBtn);
	if (revertBtn)
	{
		if (!handler || !this->property)
			revertBtn->setVisible(false);

		connect(revertBtn, &QPushButton::clicked, this, [=]() {
			if (!cdo || !this->property || !handler)
				return;

			void* newValuePtr = (void*)((SizeType)cdo + this->property->offset);
			int64 oldValue = 0;
			int64 newValue = 0;
			switch (byteSize)
			{
			case 1:
				oldValue = handler->GetValue<int8>();
				newValue = *(int8*)newValuePtr;
				break;
			case 2:
				oldValue = handler->GetValue<int16>();
				newValue = *(int16*)newValuePtr;
				break;
			case 4:
				oldValue = handler->GetValue<int32>();
				newValue = *(int32*)newValuePtr;
				break;
			case 8:
				oldValue = handler->GetValue<int64>();
				newValue = *(int64*)newValuePtr;
				break;
			}

			if (oldValue == newValue)
				return;

			curUndoCmd = new CEnumValueUndo((undoName + " Value Edited").c_str(), handler->GetValue(), byteSize, oldValue, newValue);
			handler->SetValue(newValuePtr);
			emit(OnValueChanged());
			Update();
		});
	}

	Update();
	connect(editor, SIGNAL(currentIndexChanged(int)), this, SLOT(valueChanged(int)));
}

void CEnumProperty::InitFlags(const FString& name)
{
	CCollapsableWidget* header = new CCollapsableWidget(name.c_str(), nullptr, this);
	header->SetHeaderType(CCollapsableWidget::NESTED_HEADER);
	header->GetHeader()->setMinimumHeight(32);

	QWidget* content = new QWidget(header);
	QVBoxLayout* cl = new QVBoxLayout(content);
	cl->setContentsMargins(16, 0, 0, 0);
	header->SetWidget(content);
	layout()->addWidget(header);

	for (auto& option : options)
	{
		QWidget* wd = new QWidget(content);
		QHBoxLayout* l = new QHBoxLayout(wd);
		l->setContentsMargins(0, 0, 0, 0);

		bool bChecked = *(int32*)value & option.Value;

		QCheckBox* cb = new QCheckBox(wd);
		cb->setChecked(bChecked);

		QLabel* label = new QLabel(option.Key.c_str(), wd);
		l->addWidget(label);
		l->addWidget(cb);
		cl->addWidget(wd);

		connect(cb, &QCheckBox::stateChanged, this, [=](int state) {
			int32 v = *(int32*)value;
			if (state == Qt::Checked)
				v |= option.Value;
			else
				v &= ~option.Value;
			*(int32*)value = v;
			emit(OnValueChanged());
		});
	}
}

void CEnumProperty::valueChanged(int index)
{
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

	curUndoCmd = new CEnumValueUndo((undoName + " Value Edited").c_str(), handler ? handler->GetValue() : value, byteSize, oldValue, newValue);

	if (handler)
	{
		switch (byteSize)
		{
		case 1:
		{
			int8 nv = (int8)newValue;
			handler->SetValue(&nv);
		}
			break;
		case 2:
		{
			int16 nv = (int16)newValue;
			handler->SetValue(&nv);
		}
			break;
		case 4:
		{
			int32 nv = (int32)newValue;
			handler->SetValue(&nv);
		}
			break;
		case 8:
		{
			int64 nv = (int64)newValue;
			handler->SetValue(&nv);
		}
			break;
		}
	}
	else
	{
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
	}

	if (cdo && this->property && handler && revertBtn)
	{
		bool b = handler->Equals((void*)((SizeType)cdo + this->property->offset));
		revertBtn->setVisible(!b);
	}

	emit(OnValueChanged());
}

void CEnumProperty::ResetOptions()
{
	editor->clear();
	for (auto& p : options)
		editor->addItem(p.Key.c_str(), QVariant(p.Value));
}
