
#include "ObjectPtrProperty.h"
#include "Assets/Asset.h"
#include "Widgets/FileDialogs.h"
#include "Widgets/ObjectSelectorWidget.h"
#include "Editor.h"
#include "Console.h"
#include "EditorEngine.h"
#include "Object/PropertyHandler.h"

#include <QMimeData>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDragEnterEvent>
#include <QComboBox>
#include <QUndoCommand>

CObjectPtrProperty::CObjectPtrProperty(void* v, const FProperty* property, QWidget* parent /*= nullptr*/) : IBasePropertyEditor(parent), value((TObjectPtr<CObject>*)v)
{
	this->property = property;
	handler = property->GetHandler(v);
	value = (TObjectPtr<CObject>*)handler->GetValue();

	if (property->typeName == "TObjectPtr")
		_class = CModuleManager::FindClass(property->templateType[0].typeName);
	else
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
	undoName = name + " Value Edited";

	setAcceptDrops(true);

	QLabel* label = new QLabel(name, this);

	layout->addWidget(label);
	layout->addStretch(0);

	revertBtn = AddRevertBtn(handler);

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
					TObjectPtr<CObject> oldValue = GetCurrentValue();
					TObjectPtr<CObject> newValue = &*obj;
					if (oldValue == newValue)
						return;
					curUndoCmd = makeUndo(oldValue, newValue);
					SetCurrentValue(newValue);
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
				TObjectPtr<CObject> oldValue = GetCurrentValue();
				TObjectPtr<CObject> newValue = &*CAssetManager::GetAsset((FAssetClass*)_class, edit->GetObjectId());
				if (oldValue == newValue)
					return;
				curUndoCmd = makeUndo(oldValue, newValue);
				SetCurrentValue(newValue);
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
				TObjectPtr<CObject> oldValue = GetCurrentValue();
				TObjectPtr<CObject> newValue = CObjectManager::FindObject(edit->GetObjectId());
				if (oldValue == newValue)
					return;
				curUndoCmd = makeUndo(oldValue, newValue);
				SetCurrentValue(newValue);
				emit(OnValueChanged());
			});
		});
	}

	layout->addWidget(revertBtn);
	revertBtn->setVisible(false);

	connect(revertBtn, &QPushButton::clicked, this, [=]() {
		if (!cdo || !this->property || !handler)
			return;

		TObjectPtr<CObject> oldValue = GetCurrentValue();
		TObjectPtr<CObject> newValue = *(TObjectPtr<CObject>*)((SizeType)cdo + this->property->offset);
		if (oldValue == newValue)
			return;

		curUndoCmd = makeUndo(oldValue, newValue);
		SetCurrentValue(newValue);
		emit(OnValueChanged());
		Update();
	});

	Update();
}

QUndoCommand* CObjectPtrProperty::makeUndo(const TObjectPtr<CObject>& oldValue, const TObjectPtr<CObject>& newValue)
{
	class Undo : public QUndoCommand
	{
	public:
		Undo(const QString& name, TObjectPtr<CObject>* ptr, const TObjectPtr<CObject>& oldv, const TObjectPtr<CObject>& newv)
			: QUndoCommand(name), ptr(ptr), oldValue(oldv), newValue(newv)
		{
		}

		int id() const override
		{
			return 1009;
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

		TObjectPtr<CObject>* ptr;
		TObjectPtr<CObject> oldValue;
		TObjectPtr<CObject> newValue;
	};

	return new Undo(undoName, handler ? (TObjectPtr<CObject>*)handler->GetValue() : value, oldValue, newValue);
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
		FAssetClass* type = CAssetManager::GetAssetTypeByFile(file);
		if (file && type && _class->CanCast(type))
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
		FAssetClass* type = CAssetManager::GetAssetTypeByFile(file);
		if (file && type && _class->CanCast(type))
		{
			event->acceptProposedAction();
			gEditorEngine->PushEvent(EventExec_PreUpdate, [=]() {
				TObjectPtr<CAsset> obj = CAssetManager::GetAsset((FAssetClass*)_class, file->Path());
				TObjectPtr<CObject> oldValue = GetCurrentValue();
				TObjectPtr<CObject> newValue = &*obj;
				if (oldValue == newValue)
					return;
				curUndoCmd = makeUndo(oldValue, newValue);
				SetCurrentValue(newValue);
				edit->SetObject(obj);
				emit(OnValueChanged());
			});
		}
	}
}

void CObjectPtrProperty::Update()
{
	if (handler)
		value = (TObjectPtr<CObject>*)handler->GetValue();

	if (!value)
		return;

	CObject* obj = GetCurrentValue();
	CObjectSelectorWidget* edit = (CObjectSelectorWidget*)widget;
	if (obj && obj->Id() != edit->GetObjectId())
		edit->SetObject(obj);
	else if (!obj)
		edit->SetObject(nullptr);

	if (cdo && this->property && handler && revertBtn)
	{
		bool b = handler->Equals((void*)((SizeType)cdo + this->property->offset));
		revertBtn->setVisible(!b);
	}
}

void CObjectPtrProperty::AllowNull(bool b)
{
	edit->bAllowNull = b;
}

TObjectPtr<CObject> CObjectPtrProperty::GetCurrentValue() const
{
	if (handler)
		return *(TObjectPtr<CObject>*)handler->GetValue();

	if (!value)
		return nullptr;

	return *value;
}

void CObjectPtrProperty::SetCurrentValue(const TObjectPtr<CObject>& newValue)
{
	if (handler)
	{
		auto copy = newValue;
		handler->SetValue(&copy);
		value = (TObjectPtr<CObject>*)handler->GetValue();
		return;
	}

	if (value)
		*value = newValue;
}
