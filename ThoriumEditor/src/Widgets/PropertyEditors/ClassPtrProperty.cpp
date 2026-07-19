
#include "ClassPtrProperty.h"
#include "Widgets/ClassSelectorWidget.h"
#include "EditorEngine.h"
#include "Object/PropertyHandler.h"

#include <QMimeData>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QUndoCommand>

class Undo : public QUndoCommand
{
public:
	Undo(const QString& name, FClass** ptr, FClass* oldv, FClass* newv)
		: QUndoCommand(name), ptr(ptr), oldValue(oldv), newValue(newv)
	{
	}

	int id() const override
	{
		return 1010;
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

	FClass** ptr;
	FClass* oldValue;
	FClass* newValue;
};

CClassPtrProperty::CClassPtrProperty(void* v, const FProperty* p, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent)
{
	property = p;
	filter = CModuleManager::FindClass(property->templateType[0].typeName);
	handler = property->GetHandler(v);

	Init(property->name.c_str());
}

CClassPtrProperty::CClassPtrProperty(const FString& name, void* v, FClass* clas, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), filter(clas)
{
	Init(name.c_str());
}

void CClassPtrProperty::Update()
{
	if (!handler)
		return;

	FClass* c = handler->GetValue<FClass*>();
	if (c != edit->GetClass())
		edit->SetClass(c);

	if (cdo)
	{
		bool b = handler->Equals((void*)((SizeType)cdo + property->offset));
		revertBtn->setVisible(!b);
	}
}

void CClassPtrProperty::Init(const QString& name)
{
	setProperty("type", QVariant(1));
	auto* layout = new QHBoxLayout(this);
	const QString commandName = name + " Value Edited";

	QLabel* label = new QLabel(name, this);

	layout->addWidget(label);
	layout->addStretch(0);

	edit = new CClassSelectorWidget(filter, this);
	edit->setMinimumWidth(180);
	widget = edit;

	layout->addWidget(edit);

	revertBtn = AddRevertBtn(handler);
	layout->addWidget(revertBtn);

	connect(revertBtn, &QPushButton::clicked, this, [=]() {
		if (!cdo)
			return;

		FClass** newValue = (FClass**)((SizeType)cdo + property->offset);
		curUndoCmd = new Undo(commandName, (FClass**)handler->GetValue(), handler->GetValue<FClass*>(), *newValue);
		handler->SetValue(&newValue);
		emit(OnValueChanged());
	});
	connect(edit, &CClassSelectorWidget::ClassChanged, this, [=]() {
		gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
			FClass* oldValue = handler->GetValue<FClass*>();
			FClass* newValue = edit->GetClass();
			if (oldValue == newValue)
				return;
			curUndoCmd = new Undo(commandName, (FClass**)handler->GetValue(), oldValue, newValue);
			handler->SetValue(&newValue);
			emit(OnValueChanged());
		});
	});

	Update();
}
