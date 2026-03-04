
#include "BoolProperty.h"
#include "Object/Class.h"
#include "Object/PropertyHandler.h"

#include <QLabel>
#include <QCheckBox>
#include <QBoxLayout>
#include <QVariant>
#include <QUndoCommand>
#include <QPushButton>

class Undo : public QUndoCommand
{
public:
	Undo(const QString& name, bool* ptr, bool oldv, bool newv) : QUndoCommand(name), ptr(ptr), oldValue(oldv), newValue(newv)
	{
	}

	int id() const override
	{
		return 1001;
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

	bool* ptr;
	bool oldValue;
	bool newValue;
};

CBoolProperty::CBoolProperty(bool* v, const FProperty* p, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{
	property = p;

	setProperty("type", QVariant(1));
	QHBoxLayout* l = new QHBoxLayout(this);
	setLayout(l);

	editor = new QCheckBox(this);

	handler = property->GetHandler(v);

	QLabel* label = new QLabel(property->name.c_str(), this);
	revertBtn = AddRevertBtn(handler);

	l->addWidget(label);
	l->addStretch();
	l->addWidget(editor);
	l->addWidget(revertBtn);

	Update();
	connect(revertBtn, &QPushButton::clicked, this, [=]() {
		if (!cdo)
			return;

		bool* newValue = (bool*)((SizeType)cdo + property->offset);
		curUndoCmd = new Undo((property->name + " Value Edited").c_str(), (bool*)handler->GetValue(), handler->GetValue<bool>(), *newValue);
		handler->SetValue((void*)newValue);
		emit(OnValueChanged());
		Update();
	});
	connect(editor, &QCheckBox::stateChanged, this, [=](int b) {
		bool newValue = b != 0;
		if (handler->GetValue<bool>() == newValue)
			return;
		curUndoCmd = new Undo((property->name + " Value Edited").c_str(), (bool*)handler->GetValue(), handler->GetValue<bool>(), newValue);
		//*value = newValue;
		handler->SetValue((void*)&newValue);
		emit(OnValueChanged());
	});
}

void CBoolProperty::Update()
{
	blockSignals(true);
	if (editor->isChecked() != handler->GetValue<bool>())
		editor->setChecked(handler->GetValue<bool>());

	if (cdo)
	{
		bool b = handler->Equals((void*)((SizeType)cdo + property->offset));
		revertBtn->setVisible(!b);
	}
	blockSignals(false);
}
