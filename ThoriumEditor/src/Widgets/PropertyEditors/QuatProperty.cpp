
#include "QuatProperty.h"
#include <QLabel>
#include <QDoubleSpinBox>
#include <QBoxLayout>
#include <QVariant>
#include <QUndoCommand>

CQuatProperty::CQuatProperty(FQuaternion* v, const FString& name, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value(v)
{
	setProperty("type", QVariant(1));
	auto* layout = new QHBoxLayout(this);
	undoName = name;

	QLabel* label = new QLabel(name.c_str(), this);

	layout->addWidget(label);
	layout->addStretch(0);

	FVector euler = v->ToEuler().Degrees();

	for (uint8 i = 0; i < 3; i++)
	{
		editors[i] = new QDoubleSpinBox(this);
		editors[i]->setMinimum(-99999999);
		editors[i]->setMaximum(FLT_MAX);
		
		editors[i]->setMinimumWidth(60);

		const char* names[] = {
			"X:",
			"Y:",
			"Z:"
		};

		QLabel* label = new QLabel(names[i], this);

		layout->addWidget(label);
		layout->addWidget(editors[i]);

		editors[i]->setValue(((float*)&euler)[i]);

		connect(editors[i], (void (QDoubleSpinBox::*)(double)) & QDoubleSpinBox::valueChanged, this, &CQuatProperty::Changed);
	}

	Update();
}

void CQuatProperty::Update()
{
	if (!value)
		return;

	blockSignals(true);
	if (cache != *value)
	{
		FVector euler = value->ToEuler().Degrees();

		editors[0]->setValue(euler.x);
		editors[1]->setValue(euler.y);
		editors[2]->setValue(euler.z);
		cacheEuler = euler;
		cache = *value;
	}
	blockSignals(false);
}

void CQuatProperty::Changed()
{
	class Undo : public QUndoCommand
	{
	public:
		Undo(const QString& name, FQuaternion* ptr, const FQuaternion& oldv, const FQuaternion& newv)
			: QUndoCommand(name), ptr(ptr), oldValue(oldv), newValue(newv)
		{
		}

		int id() const override
		{
			return 1008;
		}

		bool mergeWith(const QUndoCommand* other) override
		{
			auto* cmd = static_cast<const Undo*>(other);
			if (!cmd || cmd->ptr != ptr)
				return false;
			newValue = cmd->newValue;
			return true;
		}

		void undo() override
		{
			*ptr = oldValue;
		}

		void redo() override
		{
			*ptr = newValue;
		}

		FQuaternion* ptr;
		FQuaternion oldValue;
		FQuaternion newValue;
	};

	FVector euler;

	euler.x = editors[0]->value();
	euler.y = editors[1]->value();
	euler.z = editors[2]->value();

	if (euler == cacheEuler)
		return;

	int8 axis = 0;
	if (euler.y != cacheEuler.y)
		axis = 1;
	
	FVector delta = euler - cacheEuler;

	FQuaternion q;
	if (axis == 1)
		q = FQuaternion::EulerAngles(delta.Radians()) * (*value);
	else
		q = (*value) * FQuaternion::EulerAngles(delta.Radians());

	curUndoCmd = new Undo((undoName + " Value Edited").c_str(), value, *value, q);
	cache = q;
	cacheEuler = euler;
	*value = q;
	emit(OnValueChanged());
}
