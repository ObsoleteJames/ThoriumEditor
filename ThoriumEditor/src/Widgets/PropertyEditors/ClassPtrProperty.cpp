
#include "ClassPtrProperty.h"
#include "Widgets/ClassSelectorWidget.h"
#include "EditorEngine.h"

#include <QMimeData>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>

CClassPtrProperty::CClassPtrProperty(void* v, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value((FClass**)v)
{
	filter = CModuleManager::FindClass(property->typeName);

	Init(property->name.c_str());
}

CClassPtrProperty::CClassPtrProperty(const FString& name, void* v, FClass* clas, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value((FClass**)v), filter(clas)
{
	Init(name.c_str());
}

void CClassPtrProperty::Update()
{
	if (!value)
		return;

	FClass* c = *value;
	if (c != edit->GetClass())
		edit->SetClass(c);
}

void CClassPtrProperty::Init(const QString& name)
{
	setProperty("type", QVariant(1));
	auto* layout = new QHBoxLayout(this);

	QLabel* label = new QLabel(name, this);

	layout->addWidget(label);
	layout->addStretch(0);

	edit = new CClassSelectorWidget(filter, this);
	edit->setMinimumWidth(180);
	widget = edit;

	layout->addWidget(edit);

	connect(edit, &CClassSelectorWidget::ClassChanged, this, [=]() {
		gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
			FClass* c = edit->GetClass();
			*value = c;
			emit(OnValueChanged());
		});
	});

	Update();
}
