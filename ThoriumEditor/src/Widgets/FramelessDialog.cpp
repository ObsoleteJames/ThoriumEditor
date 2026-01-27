
#include "FramelessDialog.h"
#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QApplication>

CFramelessDialog::CFramelessDialog(QWidget* parent) : QDialog(parent)
{
	setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);

	QHBoxLayout* barLayout = new QHBoxLayout();

	titlebarWidget = new QWidget(this);
	titlebarWidget->setObjectName("titlebar");
	titlebarWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	titlebarWidget->setMaximumHeight(40);
	titlebarWidget->setLayout(barLayout);

	closeButton = new QPushButton(titlebarWidget);
	closeButton->setObjectName("closebtn");
	closeButton->setProperty("type", QVariant("critical"));
	closeButton->setIcon(QIcon(":/icons/cross-white.svg"));
	connect(closeButton, &QPushButton::clicked, this, &CFramelessDialog::close);

	titleLable = new QLabel(QApplication::applicationDisplayName(), titlebarWidget);
	QFont font = titleLable->font();
	font.setBold(true);
	font.setPointSize(12);
	titleLable->setFont(font);
	
	barLayout->addWidget(titleLable);
	barLayout->addStretch(1);
	barLayout->addWidget(closeButton);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setSpacing(0);
	//layout->setMargin(1);
	layout->setContentsMargins(0, 0, 0, 0);

	layout->addWidget(titlebarWidget);
}

CFramelessDialog::~CFramelessDialog()
{
}

void CFramelessDialog::setCentralWidget(QWidget* w)
{
	if (centralWidget)
		layout()->removeWidget(centralWidget);

	centralWidget = w;
	layout()->addWidget(centralWidget);
}

void CFramelessDialog::mouseMoveEvent(QMouseEvent* event)
{
	if (bMove)
		move(pos() + (event->pos() - lastMousePos));
}

void CFramelessDialog::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
		return;

	if (titlebarWidget->underMouse() || titleLable->underMouse())
	{
		bMove = true;
		lastMousePos = event->pos();
	}
}

void CFramelessDialog::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
		bMove = false;
}
