
#include "ClassSelectorWidget.h"
#include "Object/Object.h"

#include <QDialog>
#include <QListWidget>
#include <QStylePainter>
#include <QStyleOption>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QLineEdit>

class P_ClassSelectorDrowdown : public QDialog
{
public:
	P_ClassSelectorDrowdown(CClassSelectorWidget* obj) : QDialog(obj), classWidget(obj)
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

		QPoint p = classWidget->mapToGlobal(QPoint()) + QPoint(0, classWidget->height());
		QRect rect(p, QSize(classWidget->width(), 360));
		setGeometry(rect);

		connect(searchEdit, &QLineEdit::textChanged, this, [=](const QString& txt) {
			PopulateList(txt);
		});
		connect(list, &QListWidget::itemDoubleClicked, this, [=](QListWidgetItem* item) {
			selected = (FClass*)item->data(257).toULongLong();
			done(true);
		});
	}

	void PopulateList(const QString& filter = QString())
	{
		list->clear();
		
		FClass* classFilter = classWidget->filterClass ? classWidget->filterClass : CObject::StaticClass();
		
		TArray<FClass*> classes;
		CModuleManager::GetClassesOfType(classFilter, classes);

		for (auto* c : classes)
		{
			QString cName = c->GetName().c_str();
			if (!filter.isEmpty() && !cName.contains(filter))
				continue;

			if (c->HasFlag(CTAG_ABSTRACT) || c->HasFlag(CTAG_HIDDEN))
				continue;

			auto* item = new QListWidgetItem(cName, list);
			item->setData(257, (uint64)c);
		}
	}

	inline FClass* GetClass() const { return selected; }

private:
	FClass* selected;
	CClassSelectorWidget* classWidget;
	QLineEdit* searchEdit;
	QListWidget* list;
};

CClassSelectorWidget::CClassSelectorWidget(QWidget* parent /*= nullptr*/) : QWidget(parent)
{
	Init();
}

CClassSelectorWidget::CClassSelectorWidget(FClass* filter, QWidget* parent /*= nullptr*/) : QWidget(parent), filterClass(filter)
{
	Init();
}

void CClassSelectorWidget::Init()
{
	setMaximumHeight(24);
	setMinimumHeight(18);
}

void CClassSelectorWidget::SetClass(FClass* clas)
{
	this->clas = clas;
	update();
}

void CClassSelectorWidget::SetClassFilter(FClass* clas)
{
	filterClass = clas;

	if (clas && !clas->CanCast(filterClass))
	{
		clas = nullptr;
		update();
	}
}

void CClassSelectorWidget::paintEvent(QPaintEvent* event)
{
	QStylePainter painter(this);

	QString text;
	if (clas)
		text = clas->GetName().c_str();

	QStyleOptionButton opt;
	opt.initFrom(this);
	opt.features |= QStyleOptionButton::HasMenu;
	opt.text = text;
	opt.direction = Qt::LeftToRight;
	opt.rect = rect();
	QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &opt, this);
	
	painter.drawControl(QStyle::CE_PushButton, opt);

	if (!clas)
	{
		const Qt::LayoutDirection layoutDir = opt.direction;
		QPen oldPen = painter.pen();
		painter.setPen(QColor(127, 127, 127));

		painter.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, "None");
		painter.setPen(oldPen);
	}
}

void CClassSelectorWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
		return;

	P_ClassSelectorDrowdown dialog(this);
	if (dialog.exec())
	{
		clas = dialog.GetClass();
		emit(ClassChanged());
		update();
	}
}
