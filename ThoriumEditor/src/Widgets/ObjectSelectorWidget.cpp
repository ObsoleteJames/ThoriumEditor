
#include <string>
#include "ObjectSelectorWidget.h"
#include "Assets/Asset.h"

#include <QDialog>
#include <QListWidget>
#include <QStylePainter>
#include <QStyleOption>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QLineEdit>

class CObjectSelectDialog : public QDialog
{
public:
	CObjectSelectDialog(CObjectSelectorWidget* obj) : QDialog(obj), objectWidget(obj)
	{
		setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
		setMaximumHeight(500);
		auto* layout = new QVBoxLayout(this);

		searchEdit = new QLineEdit(this);
		searchEdit->setPlaceholderText("Search...");

		list = new QListWidget(this);
		
		layout->addWidget(searchEdit);
		layout->addWidget(list);

		PopulateList();

		QPoint p = objectWidget->mapToGlobal(QPoint()) + QPoint(0, objectWidget->height());
		QRect rect(p, QSize(objectWidget->width(), 360));
		setGeometry(rect);

		//connect(list, &QListWidget::itemClicked, this, [=](QListWidgetItem* item) {
		//	if (objectWidget->bTypeIsAsset)
		//	{
		//		this->obj = (CObject*)item->data(257).toULongLong();
		//	}
		//	else
		//	{

		//	}
		//});
		connect(searchEdit, &QLineEdit::textChanged, this, [=](const QString& txt) {
			PopulateList(txt);
		});
		connect(list, &QListWidget::itemDoubleClicked, this, [=](QListWidgetItem* item) {
			if (objectWidget->bTypeIsAsset)
			{
				QString path = item->data(257).toString();
				if (!path.isEmpty())
				{
					//FString p = path.toStdString();
					//this->obj = CAssetManager::GetAsset((FAssetClass*)objectWidget->GetClassFilter(), p);
					objId = item->data(257).toULongLong();
				}
				else
					this->objId = 0;
			}
			else
				this->objId = item->data(257).toULongLong();

			done(true); 
		});
	}

	void PopulateList(const QString& filter = QString())
	{
		list->clear();
		if (objectWidget->bAllowNull)
			list->addItem("None");

		if (!objectWidget->bTypeIsAsset)
		{
			const TMap<FGuid, CObject*>& objects = CObjectManager::GetAllObjects();
			for (auto& it : objects)
			{
				if (it.second->GetClass()->CanCast(objectWidget->GetClassFilter()))
				{
					if (objectWidget->validator && !objectWidget->validator(it.second))
						continue;

					QString text = it.second->Name().c_str();
					if (!filter.isEmpty() && !text.contains(filter))
						continue;

					auto* item = new QListWidgetItem(text, list);
					item->setData(257, (uint64)it.second->Id());
				}
			}
		}
		else
		{
			auto& assets = CAssetManager::GetAssetsData();
			for (auto& it : assets)
			{
				if (it.second.type == objectWidget->GetClassFilter())
				{
					if (objectWidget->assetValidator && !objectWidget->assetValidator(it.second.file->Path()))
						continue;

					QString text = QString(it.second.file->Name().c_str());
					if (!filter.isEmpty() && !text.contains(filter))
						continue;

					auto* item = new QListWidgetItem(text, list);
					item->setData(257, (uint64)it.second.id);
					//auto* item = new QListWidgetItem(text, list);
					//item->setData(257, QString(it.second.file->Path().c_str()));
				}
			}
		}
	}

	//inline CObject* GetObject() const { return obj; }
	inline SizeType GetObject() const { return objId; }

private:
	//CObject* obj;
	SizeType objId;
	CObjectSelectorWidget* objectWidget;
	QLineEdit* searchEdit;
	QListWidget* list;

};

CObjectSelectorWidget::CObjectSelectorWidget(QWidget* parent /*= nullptr*/) : QWidget(parent)
{
	Init();
}

CObjectSelectorWidget::CObjectSelectorWidget(FClass* filter, QWidget* parent /*= nullptr*/) : QWidget(parent), filterType(filter)
{
	Init();
}

CObjectSelectorWidget::CObjectSelectorWidget(CObject* obj, FClass* filter, QWidget* parent /*= nullptr*/) : QWidget(parent), filterType(filter)
{
	Init();
	SetObject(obj);
}

void CObjectSelectorWidget::Init()
{
	setMaximumHeight(24);
	setMinimumHeight(18);
	
	validator = &CObjectSelectorWidget::DefaultValidator;

	if (filterType)
		bTypeIsAsset = filterType->CanCast(CAsset::StaticClass());
}

void CObjectSelectorWidget::SetObject(CObject* obj)
{
	objId = 0;
	if (obj)
	{
		this->objId = obj->Id();
		if (bTypeIsAsset)
		{
			TObjectPtr<CAsset> asset = Cast<CAsset>(obj);
			if (asset->File())
				text = QString(asset->File()->Name().c_str());
			else
				text = obj->Name().c_str();
		}
		else
			text = obj->Name().c_str();
	}

	update();
}

void CObjectSelectorWidget::SetObject(SizeType obj)
{
	objId = obj;
	if (objId != 0)
	{
		if (bTypeIsAsset)
		{
			const FAssetData* asset = CAssetManager::GetAssetData(objId);
			if (asset)
				text = asset->file->Name().c_str();
		}
		else
		{
			CObject* object = CObjectManager::FindObject(objId);
			if (object)
				text = object->Name().c_str();
		}
	}

	update();
}

void CObjectSelectorWidget::SetClassFilter(FClass* type)
{
	filterType = type;
	bTypeIsAsset = filterType->CanCast(CAsset::StaticClass());

	if (objId != 0)
	{
		objId = 0;
		update();
	}
}

void CObjectSelectorWidget::paintEvent(QPaintEvent* event)
{
	QStylePainter painter(this);
	
	//QString text;
	//if (obj)
	//{
	//	if (bTypeIsAsset)
	//	{
	//		TObjectPtr<CAsset> asset = Cast<CAsset>(obj);
	//		if (asset->File())
	//			text = QString(asset->File()->Name().c_str());
	//		else
	//			text = obj->Name().c_str();
	//	}
	//	else
	//		text = obj->Name().c_str();
	//}

	QStyleOptionButton opt;
	opt.initFrom(this);
	opt.features |= QStyleOptionButton::HasMenu;
	opt.text = text;
	opt.direction = Qt::LeftToRight;
	opt.rect = rect();
	QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &opt, this);

	painter.drawControl(QStyle::CE_PushButton, opt);

	if (objId == 0)
	{
		const Qt::LayoutDirection layoutDir = opt.direction;
		QPen oldPen = painter.pen();
		painter.setPen(QColor(127, 127, 127));

		painter.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, "None");
		painter.setPen(oldPen);
	}
}

void CObjectSelectorWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
		return;

	CObjectSelectDialog dialog(this);
	if (dialog.exec())
	{
		//obj = dialog.GetObject();
		SetObject(dialog.GetObject());
		emit(ObjectChanged());
		//update();
	}
}
