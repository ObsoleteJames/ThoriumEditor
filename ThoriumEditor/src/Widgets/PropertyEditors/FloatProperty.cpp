
#include "FloatProperty.h"
#include "Object/Class.h"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QBoxLayout>
#include <QVariant>
#include <QUndoCommand>

CFloatProperty::CFloatProperty(void* value, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{
	setProperty("type", QVariant(1));
	setLayout(new QHBoxLayout());
	undoName = property->name;

	editor = new QDoubleSpinBox(this);

	if (property->type == EVT_FLOAT)
	{
		vFloat = (float*)value;
		bDouble = false;
	}
	else
	{
		vDouble = (double*)value;
		bDouble = true;
	}

	editor->setMinimum(FLT_MIN);
	editor->setMaximum(FLT_MAX);
	editor->setSingleStep(0.1);

	QLabel* label = new QLabel(property->name.c_str());

	layout()->addWidget(label);
	layout()->addWidget(editor);

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

	QLabel* label = new QLabel(name.c_str());

	layout()->addWidget(label);
	layout()->addWidget(editor);

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

	QLabel* label = new QLabel(name.c_str());

	layout()->addWidget(label);
	layout()->addWidget(editor);

	Update();
	connect(editor, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)));
}

void CFloatProperty::Update()
{
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
	class Undo : public QUndoCommand
	{
	public:
		Undo(const QString& name, float* fptr, double* dptr, bool isDouble, double oldv, double newv)
			: QUndoCommand(name), fptr(fptr), dptr(dptr), isDouble(isDouble), oldValue(oldv), newValue(newv)
		{
		}

		int id() const override
		{
			return 1002;
		}

		bool mergeWith(const QUndoCommand* other) override
		{
			auto* cmd = static_cast<const Undo*>(other);
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

	const QString commandName = (undoName + " Value Edited").c_str();
	if (bDouble)
	{
		if (*vDouble == v)
			return;
		curUndoCmd = new Undo(commandName, nullptr, vDouble, true, *vDouble, v);
		*vDouble = v;
	}
	else
	{
		if (*vFloat == (float)v)
			return;
		curUndoCmd = new Undo(commandName, vFloat, nullptr, false, *vFloat, v);
		*vFloat = (float)v;
	}
	emit(OnValueChanged());
}
