
#include "IntProperty.h"
#include "Object/Class.h"
#include "Object/PropertyHandler.h"

#include <QLabel>
#include <QSpinBox>
#include <QBoxLayout>
#include <QVariant>
#include <QUndoCommand>
#include <QPushButton>

class CIntValueUndo : public QUndoCommand
{
public:
	CIntValueUndo(const QString& name, int* ptr, uint8 byteSize, int64 oldv, int64 newv)
		: QUndoCommand(name), ptr(ptr), byteSize(byteSize), oldValue(oldv), newValue(newv)
	{
	}

	int id() const override
	{
		return 1003;
	}

	bool mergeWith(const QUndoCommand* other) override
	{
		auto* cmd = static_cast<const CIntValueUndo*>(other);
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

	int* ptr;
	uint8 byteSize;
	int64 oldValue;
	int64 newValue;
};

class CUIntValueUndo : public QUndoCommand
{
public:
	CUIntValueUndo(const QString& name, uint* ptr, uint8 byteSize, uint64 oldv, uint64 newv)
		: QUndoCommand(name), ptr(ptr), byteSize(byteSize), oldValue(oldv), newValue(newv)
	{
	}

	int id() const override
	{
		return 1004;
	}

	bool mergeWith(const QUndoCommand* other) override
	{
		auto* cmd = static_cast<const CUIntValueUndo*>(other);
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
			*(uint8*)ptr = (uint8)oldValue;
			break;
		case 2:
			*(uint16*)ptr = (uint16)oldValue;
			break;
		case 4:
			*ptr = (uint32)oldValue;
			break;
		case 8:
			*(SizeType*)ptr = (SizeType)oldValue;
			break;
		}
	}

	void redo() override
	{
		switch (byteSize)
		{
		case 1:
			*(uint8*)ptr = (uint8)newValue;
			break;
		case 2:
			*(uint16*)ptr = (uint16)newValue;
			break;
		case 4:
			*ptr = (uint32)newValue;
			break;
		case 8:
			*(SizeType*)ptr = (SizeType)newValue;
			break;
		}
	}

	uint* ptr;
	uint8 byteSize;
	uint64 oldValue;
	uint64 newValue;
};

CIntProperty::CIntProperty(int* v, const FProperty* property, QWidget* parent) : IBasePropertyEditor(parent), value(v)
{
	this->property = property;
	byteSize = property->size;
	setProperty("type", QVariant(1));
	undoName = property->name;
	handler = property->GetHandler(v);
	value = (int*)handler->GetValue();
	
	setLayout(new QHBoxLayout());
	
	editor = new QSpinBox(this);
	
	int min = INT32_MIN, max = INT32_MAX;
	switch (byteSize)
	{
	case 1:
		min = INT8_MIN;
		max = INT8_MAX;
		break;
	case 2:
		min = INT16_MIN;
		max = INT16_MAX;
		break;
	}

	editor->setMinimum(min);
	editor->setMaximum(max);

	QLabel* label = new QLabel(property->name.c_str(), this);
	
	layout()->addWidget(label);
	layout()->addWidget(editor);

	revertBtn = AddRevertBtn(handler);
	layout()->addWidget(revertBtn);
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

		curUndoCmd = new CIntValueUndo((undoName + " Value Edited").c_str(), (int*)handler->GetValue(), byteSize, oldValue, newValue);
		handler->SetValue(newValuePtr);
		emit(OnValueChanged());
		Update();
	});

	Update();
	connect(editor, SIGNAL(valueChanged(int)), this, SLOT(onValueChanged(int)));
}

CIntProperty::CIntProperty(const FString& name, int* v, int min /*= 0*/, int max /*= 0*/, QWidget* parent) : IBasePropertyEditor(parent), value(v)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());
	undoName = name;

	editor = new QSpinBox(this);

	if (min == 0 && max == 0)
	{
		min = INT32_MIN;
		max = INT32_MAX;
	}

	editor->setMinimum(min);
	editor->setMaximum(max);

	QLabel* label = new QLabel(name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	revertBtn = AddRevertBtn(nullptr);
	layout()->addWidget(revertBtn);
	revertBtn->setVisible(false);

	Update();
	connect(editor, SIGNAL(valueChanged(int)), this, SLOT(onValueChanged(int)));
}

void CIntProperty::Update()
{
	if (handler)
		value = (int*)handler->GetValue();

	switch (byteSize)
	{
	case 1:
	{
		if (editor->value() != *(int8*)value)
			editor->setValue(*value);
	}
	break;
	case 2:
	{
		if (editor->value() != *(int16*)value)
			editor->setValue(*value);
	}
	break;
	case 4:
	{
		if (editor->value() != *(uint32*)value)
			editor->setValue(*value);
	}
	case 8:
	{
		if (editor->value() != *(int64*)value)
			editor->setValue(*value);
	}
	break;
	}
}

void CIntProperty::onValueChanged(int v)
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
	if (oldValue == v)
		return;

	curUndoCmd = new CIntValueUndo((undoName + " Value Edited").c_str(), handler ? (int*)handler->GetValue() : value, byteSize, oldValue, v);

	if (handler)
	{
		switch (byteSize)
		{
		case 1:
		{
			int8 nv = (int8)v;
			handler->SetValue(&nv);
		}
			break;
		case 2:
		{
			int16 nv = (int16)v;
			handler->SetValue(&nv);
		}
			break;
		case 4:
		{
			int32 nv = (int32)v;
			handler->SetValue(&nv);
		}
			break;
		case 8:
		{
			int64 nv = (int64)v;
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
			*(int8*)value = v;
			break;
		case 2:
			*(int16*)value = v;
			break;
		case 4:
			*(int32*)value = v;
			break;
		case 8:
			*(int64*)value = v;
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

CUIntProperty::CUIntProperty(uint* v, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value(v)
{
	this->property = property;
	byteSize = property->size;
	setProperty("type", QVariant(2));
	undoName = property->name;
	handler = property->GetHandler(v);
	value = (uint*)handler->GetValue();

	setLayout(new QHBoxLayout());

	editor = new QSpinBox(this);

	int min = 0, max = UINT32_MAX;
	switch (byteSize)
	{
	case 1:
		max = INT8_MAX;
		break;
	case 2:
		max = INT16_MAX;
		break;
	}

	editor->setMinimum(min);
	editor->setMaximum(max);

	QLabel* label = new QLabel(property->name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	revertBtn = AddRevertBtn(handler);
	layout()->addWidget(revertBtn);
	connect(revertBtn, &QPushButton::clicked, this, [=]() {
		if (!cdo || !this->property || !handler)
			return;

		void* newValuePtr = (void*)((SizeType)cdo + this->property->offset);
		uint64 oldValue = 0;
		uint64 newValue = 0;
		switch (byteSize)
		{
		case 1:
			oldValue = handler->GetValue<uint8>();
			newValue = *(uint8*)newValuePtr;
			break;
		case 2:
			oldValue = handler->GetValue<uint16>();
			newValue = *(uint16*)newValuePtr;
			break;
		case 4:
			oldValue = handler->GetValue<uint32>();
			newValue = *(uint32*)newValuePtr;
			break;
		case 8:
			oldValue = handler->GetValue<SizeType>();
			newValue = *(SizeType*)newValuePtr;
			break;
		}

		if (oldValue == newValue)
			return;

		curUndoCmd = new CUIntValueUndo((undoName + " Value Edited").c_str(), (uint*)handler->GetValue(), byteSize, oldValue, newValue);
		handler->SetValue(newValuePtr);
		emit(OnValueChanged());
		Update();
	});

	Update();
	connect(editor, SIGNAL(valueChanged(int)), this, SLOT(onValueChanged(int)));
}

CUIntProperty::CUIntProperty(const FString& name, uint* v, uint min /*= 0*/, uint max /*= 0*/, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value(v)
{
	setProperty("type", QVariant(2));
	setLayout(new QHBoxLayout());
	undoName = name;

	editor = new QSpinBox(this);

	if (min == 0 && max == 0)
		max = INT32_MAX;

	editor->setMinimum(min);
	editor->setMaximum(max);

	QLabel* label = new QLabel(name.c_str(), this);

	layout()->addWidget(label);
	layout()->addWidget(editor);

	revertBtn = AddRevertBtn(nullptr);
	layout()->addWidget(revertBtn);
	revertBtn->setVisible(false);

	Update();
	connect(editor, SIGNAL(valueChanged(int)), this, SLOT(onValueChanged(int)));
}

void CUIntProperty::Update()
{
	if (handler)
		value = (uint*)handler->GetValue();

	switch (byteSize)
	{
	case 1:
	{
		if (editor->value() != *(uint8*)value)
			editor->setValue(*value);
	}
		break;
	case 2:
	{
		if (editor->value() != *(uint16*)value)
			editor->setValue(*value);
	}
		break;
	case 4:
	{
		if (editor->value() != *value)
			editor->setValue(*value);
	}
	case 8:
	{
		if (editor->value() != *(SizeType*)value)
			editor->setValue(*value);
	}
		break;
	}
}

void CUIntProperty::onValueChanged(int v)
{
	uint64 oldValue = 0;
	switch (byteSize)
	{
	case 1:
		oldValue = *(uint8*)value;
		break;
	case 2:
		oldValue = *(uint16*)value;
		break;
	case 4:
		oldValue = *value;
		break;
	case 8:
		oldValue = *(SizeType*)value;
		break;
	}
	if (oldValue == (uint64)v)
		return;

	curUndoCmd = new CUIntValueUndo((undoName + " Value Edited").c_str(), handler ? (uint*)handler->GetValue() : value, byteSize, oldValue, (uint64)v);

	if (handler)
	{
		switch (byteSize)
		{
		case 1:
		{
			uint8 nv = (uint8)v;
			handler->SetValue(&nv);
		}
			break;
		case 2:
		{
			uint16 nv = (uint16)v;
			handler->SetValue(&nv);
		}
			break;
		case 4:
		{
			uint32 nv = (uint32)v;
			handler->SetValue(&nv);
		}
			break;
		case 8:
		{
			SizeType nv = (SizeType)v;
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
			*(uint8*)value = v;
			break;
		case 2:
			*(uint16*)value = v;
			break;
		case 4:
			*value = v;
			break;
		case 8:
			*(SizeType*)value = v;
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
