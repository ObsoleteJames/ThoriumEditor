
#include "VectorProperty.h"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QBoxLayout>
#include <QVariant>
#include <QUndoCommand>

CVectorProperty::CVectorProperty(FVector* v, const FString& name, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value((float*)v)
{
	setProperty("type", QVariant(1));
	auto* layout = new QHBoxLayout(this);

	QLabel* label = new QLabel(name.c_str(), this);
	const QString commandName = (name + " Value Edited").c_str();

	layout->addWidget(label);
	layout->addStretch(0);

	for (uint8 i = 0; i < 3; i++)
	{
		editors[i] = new QDoubleSpinBox(this);
		editors[i]->setMinimum(-99999999);
		editors[i]->setMaximum(FLT_MAX);
		editors[i]->setSingleStep(0.1);

		editors[i]->setMinimumWidth(60);

		const char* names[] = {
			"X:",
			"Y:",
			"Z:"
		};

		QLabel* label = new QLabel(names[i], this);

		layout->addWidget(label);
		layout->addWidget(editors[i]);

		connect(editors[i], (void (QDoubleSpinBox::*)(double))&QDoubleSpinBox::valueChanged, this, [=](double v) {
			class Undo : public QUndoCommand
			{
			public:
				Undo(const QString& name, float* ptr, const float oldv[3], const float newv[3])
					: QUndoCommand(name), ptr(ptr)
				{
					for (int j = 0; j < 3; j++)
					{
						oldValue[j] = oldv[j];
						newValue[j] = newv[j];
					}
				}

				int id() const override
				{
					return 1007;
				}

				bool mergeWith(const QUndoCommand* other) override
				{
					auto* cmd = static_cast<const Undo*>(other);
					if (!cmd || cmd->ptr != ptr)
						return false;
					for (int j = 0; j < 3; j++)
						newValue[j] = cmd->newValue[j];
					return true;
				}

				void undo() override
				{
					for (int j = 0; j < 3; j++)
						ptr[j] = oldValue[j];
				}

				void redo() override
				{
					for (int j = 0; j < 3; j++)
						ptr[j] = newValue[j];
				}

				float* ptr;
				float oldValue[3];
				float newValue[3];
			};

			float oldValue[3] = { value[0], value[1], value[2] };
			if (oldValue[i] == (float)v)
				return;
			value[i] = (float)v;
			float newValue[3] = { value[0], value[1], value[2] };
			curUndoCmd = new Undo(commandName, value, oldValue, newValue);
			emit(OnValueChanged());
		});
	}

	Update();
}

void CVectorProperty::Update()
{
	for (uint8 i = 0; i < 3; i++)
	{
		if (editors[i]->value() != value[i])
			editors[i]->setValue(value[i]);
	}
}
