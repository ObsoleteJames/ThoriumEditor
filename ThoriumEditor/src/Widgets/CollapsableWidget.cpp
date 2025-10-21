
#include "CollapsableWidget.h"

#include <QStyle>
#include <QBoxLayout>
#include <QLabel>
#include <QVariant>
#include <QPushButton>

CCollapsableWidget::CCollapsableWidget(QWidget* parent /*= nullptr*/) : QWidget(parent), widget(nullptr)
{
	QVBoxLayout* pLayout = new QVBoxLayout(this);
	
	header = new QPushButton(this);
	header->setProperty("type", QVariant("header"));
	header->setProperty("collapsed", true);
	pLayout->addWidget(header);

	connect(header, &QPushButton::clicked, this, [=](bool) { SetCollapsed(!bCollapsed); });

	pLayout->setContentsMargins(QMargins(0, 0, 0, 0));
	pLayout->setSpacing(0);
}

CCollapsableWidget::CCollapsableWidget(QWidget* w, QWidget* parent) : QWidget(parent), widget(nullptr)
{
	QVBoxLayout* pLayout = new QVBoxLayout(this);

	header = new QPushButton(this);
	header->setProperty("type", QVariant("header"));
	header->setProperty("collapsed", true);
	pLayout->addWidget(header);

	SetWidget(w);

	connect(header, &QPushButton::clicked, this, [=](bool) { SetCollapsed(!bCollapsed); });

	pLayout->setContentsMargins(QMargins(0, 0, 0, 0));
	pLayout->setSpacing(0);
}

CCollapsableWidget::CCollapsableWidget(const QString& text, QWidget* w /*= nullptr*/, QWidget* parent /*= nullptr*/) : QWidget(parent)
{
	QVBoxLayout* pLayout = new QVBoxLayout(this);

	header = new QPushButton(this);
	header->setProperty("type", QVariant("header"));
	header->setProperty("collapsed", true);
	pLayout->addWidget(header);

	SetWidget(w);
	header->setText(text);

	connect(header, &QPushButton::clicked, this, [=](bool) { SetCollapsed(!bCollapsed); });

	pLayout->setContentsMargins(QMargins(0, 0, 0, 0));
	pLayout->setSpacing(0);
}

CCollapsableWidget::~CCollapsableWidget()
{
	delete header;
	delete layout();
	delete widget;
}

void CCollapsableWidget::SetWidget(QWidget* w)
{
	if (widget)
		layout()->removeWidget(widget);

	widget = w;
	if (widget)
	{
		layout()->addWidget(widget);
		widget->setVisible(!bCollapsed);
	}
}

void CCollapsableWidget::SetCollapsed(bool b)
{
	bCollapsed = b;
	header->setProperty("collapsed", b);
	header->style()->unpolish(header);
	header->style()->polish(header);
	widget->setVisible(!bCollapsed);
}

void CCollapsableWidget::SetHeaderType(EHeaderType type)
{
	if (type == ROOT_HEADER)
		header->setProperty("type", QVariant("header"));
	else
		header->setProperty("type", QVariant("header2"));
}

void CCollapsableWidget::SetText(const QString& text)
{
	header->setText(text);
}

QString CCollapsableWidget::Text() const
{
	return header->text();
}

void CCollapsableWidget::SetHeaderSize(const QRect& size)
{
	header->setGeometry(size);
}
