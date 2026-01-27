
#include "ObjectPtrProperty.h"
#include "Assets/Asset.h"
#include "Widgets/FileDialogs.h"
#include "Widgets/ObjectSelectorWidget.h"
#include "Editor.h"
#include "Console.h"
#include "EditorEngine.h"

#include <QMimeData>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDragEnterEvent>
#include <QComboBox>

CObjectPtrProperty::CObjectPtrProperty(void* v, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value((TObjectPtr<CObject>*)v)
{
	_class = CModuleManager::FindClass(property->typeName);
	if (_class)
		bIsAsset = _class->CanCast(CAsset::StaticClass());

	Init(property->name.c_str());
}

CObjectPtrProperty::CObjectPtrProperty(const FString& name, void* v, FClass* c, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value((TObjectPtr<CObject>*)v), _class(c)
{
	bIsAsset = _class->CanCast(CAsset::StaticClass());
	Init(name.c_str());
}

void CObjectPtrProperty::Init(const QString& name)
{
	setProperty("type", QVariant(1));
	auto* layout = new QHBoxLayout(this);

	setAcceptDrops(true);

	QLabel* label = new QLabel(name, this);

	layout->addWidget(label);
	layout->addStretch(0);

	if (bIsAsset)
	{
		QVBoxLayout* l1 = new QVBoxLayout();
		l1->setContentsMargins(0,0,0,0);

		QHBoxLayout* l2 = new QHBoxLayout();
		l2->setContentsMargins(0, 0, 0, 0);

		//QLineEdit* edit = new QLineEdit(this);
		//widget = edit;
		//edit->setMinimumWidth(180);

		edit = new CObjectSelectorWidget(_class, this);
		edit->setMinimumWidth(180);
		widget = edit;

		QPushButton* browse = new QPushButton("Browse", this);

		l2->addWidget(edit);
		l2->addWidget(browse);

		l1->addLayout(l2);
		l1->addStretch(1);
		layout->addLayout(l1);

		connect(browse, &QPushButton::clicked, this, [=]() {
			COpenFileDialog dialog((FAssetClass*)_class, this);
			if (dialog.exec() && dialog.File())
			{
				FFile* f = dialog.File();
				gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
					TObjectPtr<CAsset> obj = CAssetManager::GetAsset((FAssetClass*)_class, f->Path());
					*value = &*obj;
					edit->SetObject(obj);
					emit(OnValueChanged());
				});
			}
		});
		connect(edit, &CObjectSelectorWidget::ObjectChanged, this, [=]() {
			//QString t = edit->text();
			//if (t.isEmpty())
			//{
			//	*value = nullptr;
			//	return;
			//}
			//CAsset* obj = CastChecked<CAsset>(edit->GetObject());
			gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
				*value = &*CAssetManager::GetAsset((FAssetClass*)_class, edit->GetObjectId());
				emit(OnValueChanged());
			});
		});
	}
	else
	{
		edit = new CObjectSelectorWidget(_class, this);
		edit->setMinimumWidth(180);
		widget = edit;

		layout->addWidget(edit);

		connect(edit, &CObjectSelectorWidget::ObjectChanged, this, [=]() {
			//CAsset* obj = CastChecked<CAsset>(edit->GetObject());
			gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
				*value = CObjectManager::FindObject(edit->GetObjectId());
				emit(OnValueChanged());
			});
		});
	}

	Update();
}

void CObjectPtrProperty::dragEnterEvent(QDragEnterEvent* event)
{
	if (!bIsAsset)
		return;

	QListWidget* lw = qobject_cast<QListWidget*>(event->source());
	if (!lw)
		return;

	QListWidgetItem* item = lw->currentItem();
	if (item && item->type() == EItemTypes_AssetFile)
	{
		FFile* file = (FFile*)item->data(257).toULongLong();
		if (file && file->Extension() == ToWString(((FAssetClass*)_class)->GetExtension()))
		{
			event->acceptProposedAction();
		}
	}
	//CONSOLE_LogInfo(event->source()->objectName().toStdString());
}

void CObjectPtrProperty::dropEvent(QDropEvent* event)
{
	if (!bIsAsset)
		return;

	QListWidget* lw = qobject_cast<QListWidget*>(event->source());
	if (!lw)
		return;

	QListWidgetItem* item = lw->currentItem();
	if (item && item->type() == EItemTypes_AssetFile)
	{
		FFile* file = (FFile*)item->data(257).toULongLong();
		if (file && file->Extension() == ToWString(((FAssetClass*)_class)->GetExtension()))
		{
			event->acceptProposedAction();
			gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
				TObjectPtr<CAsset> obj = CAssetManager::GetAsset((FAssetClass*)_class, file->Path());
				*value = &*obj;
				edit->SetObject(obj);
				emit(OnValueChanged());
			});
		}
	}
}

void CObjectPtrProperty::Update()
{
	if (!value)
		return;

	CObject* obj = (*value);
	CObjectSelectorWidget* edit = (CObjectSelectorWidget*)widget;
	if (obj && obj->Id() != edit->GetObjectId())
		edit->SetObject(obj);
	else if (!obj)
		edit->SetObject(nullptr);
}

void CObjectPtrProperty::AllowNull(bool b)
{
	edit->bAllowNull = b;
}
