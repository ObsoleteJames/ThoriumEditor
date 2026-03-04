
#include "FloatProperty.h"
#include "Object/Class.h"
#include "Object/PropertyHandler.h"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QBoxLayout>
#include <QVariant>
#include <QUndoCommand>
#include <QPushButton>

class CFloatValueUndo : public QUndoCommand
{
public:
	CFloatValueUndo(const QString& name, float* fptr, double* dptr, bool isDouble, double oldv, double newv)
		: QUndoCommand(name), fptr(fptr), dptr(dptr), isDouble(isDouble), oldValue(oldv), newValue(newv)
	{
	}

	int id() const override
	{
		return 1002;
	}

	bool mergeWith(const QUndoCommand* other) override
	{
		auto* cmd = static_cast<const CFloatValueUndo*>(other);
		if (!cmd)
			return false;
		if (isDouble != cmd->isDouble)
			return false;
		if (isDouble && dptr != cmd->dptr)
			return false;
		if (!isDouble && fptr != cmd->fptr)
			return false;
		newValue = cmd->newValue;
		return true;
	}

	void undo() override
	{
		if (isDouble)
			*dptr = oldValue;
		else
			*fptr = (float)oldValue;
	}

	void redo() override
	{
		if (isDouble)
			*dptr = newValue;
		else
			*fptr = (float)newValue;
	}

	float* fptr;
	double* dptr;
	bool isDouble;
	double oldValue;
	double newValue;
};

CFloatProperty::CFloatProperty(void* value, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{
	this->property = property;
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());
	undoName = property->name;

	editor = new QDoubleSpinBox(this);
	handler = property->GetHandler(value);

	if (property->type == EVT_FLOAT)
	{
		vFloat = (float*)handler->GetValue();
		bDouble = false;
	}
	else
	{
		vDouble = (double*)handler->GetValue();
		bDouble = true;
	}

	editor->setMinimum(FLT_MIN);
	editor->setMaximum(FLT_MAX);
	editor->setSingleStep(0.1);
	editor->setDecimals(5);

	QLabel* label = new QLabel(property->name.c_str());

	layout()->addWidget(label);
	layout()->addWidget(editor);

	revertBtn = AddRevertBtn(handler);
	layout()->addWidget(revertBtn);
	connect(revertBtn, &QPushButton::clicked, this, [=]() {
		if (!cdo || !this->property || !handler)
			return;

		void* newValue = (void*)((SizeType)cdo + this->property->offset);
		if (bDouble)
		{
			double curValue = handler->GetValue<double>();
			double defaultValue = *(double*)newValue;
			if (curValue == defaultValue)
				return;
			curUndoCmd = new CFloatValueUndo((undoName + " Value Edited").c_str(), nullptr, (double*)handler->GetValue(), true, curValue, defaultValue);
		}
		else
		{
			float curValue = handler->GetValue<float>();
			float defaultValue = *(float*)newValue;
			if (curValue == defaultValue)
				return;
			curUndoCmd = new CFloatValueUndo((undoName + " Value Edited").c_str(), (float*)handler->GetValue(), nullptr, false, curValue, defaultValue);
		}

		handler->SetValue(newValue);
		emit(OnValueChanged());
		Update();
	});

	Update();
	connect(editor, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)));
}

CFloatProperty::CFloatProperty(const FString& name, float* value, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), vFloat(value), bDouble(false)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());
	undoName = name;

	editor = new QDoubleSpinBox(this);

	editor->setMinimum(FLT_MIN);
	editor->setMaximum(FLT_MAX);
	editor->setSingleStep(0.1);
	editor->setDecimals(5);

	QLabel* label = new QLabel(name.c_str());

	layout()->addWidget(label);
	layout()->addWidget(editor);

	revertBtn = AddRevertBtn(nullptr);
	layout()->addWidget(revertBtn);
	revertBtn->setVisible(false);

	Update();
	connect(editor, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)));
}

CFloatProperty::CFloatProperty(const FString& name, double* value, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), vDouble(value), bDouble(true)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());
	undoName = name;

	editor = new QDoubleSpinBox(this);

	editor->setMinimum(FLT_MIN);
	editor->setMaximum(FLT_MAX);
	editor->setSingleStep(0.1);
	editor->setDecimals(5);

	QLabel* label = new QLabel(name.c_str());

	layout()->addWidget(label);
	layout()->addWidget(editor);

	revertBtn = AddRevertBtn(nullptr);
	layout()->addWidget(revertBtn);
	revertBtn->setVisible(false);

	Update();
	connect(editor, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)));
}

void CFloatProperty::Update()
{
	if (handler)
	{
		if (bDouble)
		{
			double current = handler->GetValue<double>();
			if (editor->value() != current)
				editor->setValue(current);
		}
		else
		{
			float current = handler->GetValue<float>();
			if (editor->value() != current)
				editor->setValue(current);
		}

		if (cdo && this->property && revertBtn)
		{
			bool b = handler->Equals((void*)((SizeType)cdo + this->property->offset));
			revertBtn->setVisible(!b);
		}
		return;
	}

	if (bDouble)
	{
		if (editor->value() != *vDouble)
			editor->setValue(*vDouble);
	}
	else
	{
		if (editor->value() != *vFloat)
			editor->setValue(*vFloat);
	}
}

void CFloatProperty::valueChanged(double v)
{
	const QString commandName = (undoName + " Value Edited").c_str();
	if (handler)
	{
		if (bDouble)
		{
			double oldValue = handler->GetValue<double>();
			if (oldValue == v)
				return;
			curUndoCmd = new CFloatValueUndo(commandName, nullptr, (double*)handler->GetValue(), true, oldValue, v);
			handler->SetValue(&v);
		}
		else
		{
			float oldValue = handler->GetValue<float>();
			if (oldValue == (float)v)
				return;
			float newValue = (float)v;
			curUndoCmd = new CFloatValueUndo(commandName, (float*)handler->GetValue(), nullptr, false, oldValue, v);
			handler->SetValue(&newValue);
		}
		emit(OnValueChanged());
		return;
	}

	if (bDouble)
	{
		if (*vDouble == v)
			return;
		curUndoCmd = new CFloatValueUndo(commandName, nullptr, vDouble, true, *vDouble, v);
		*vDouble = v;
	}
	else
	{
		if (*vFloat == (float)v)
			return;
		curUndoCmd = new CFloatValueUndo(commandName, vFloat, nullptr, false, *vFloat, v);
		*vFloat = (float)v;
	}
	emit(OnValueChanged());
}
