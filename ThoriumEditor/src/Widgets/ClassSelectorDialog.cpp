
#include "ClassSelectorDialog.h"
#include "Object/Object.h"
#include "Object/Class.h"
#include "Module.h"
#include "Console.h"

#include <QBoxLayout>
#include <QTreeWidget>
#include <QListView>
#include <QSpacerItem>
#include <QPushButton>
#include "Widgets/TreeDataItem.h"

//class CClassTreeWidgetItem : public QTreeWidgetItem
//{
//public:
//	CClassTreeWidgetItem(FClass* c, QTreeWidgetItem* parent) : QTreeWidgetItem(parent, 0), clas(c) {}
//
//	inline FClass* GetClass() const { return clas; }
//
//private:
//	FClass* clas;
//};

CClassSelectorDialog::CClassSelectorDialog(QWidget* parent /*= nullptr*/, Qt::WindowFlags f /*= Qt::WindowFlags()*/) : CFramelessDialog(parent)
{
	QFrame* frame = new QFrame(this);
	QVBoxLayout* layout = new QVBoxLayout(frame);
	//{
	//	QVBoxLayout* l = new QVBoxLayout(this);
	//	l->setContentsMargins(0, 0, 0, 0);
	//	l->addWidget(frame);
	//}

	setCentralWidget(frame);
	setTitle("Select Class...");

	filterClass = nullptr;
	selectedClass = nullptr;

	classTree = new QTreeWidget(this);
	classTree->setColumnCount(1);
	classTree->setHeaderLabel("Class");

	QWidget* buttonsArea = new QWidget(this);
	QHBoxLayout* buttonsLayout = new QHBoxLayout(this);
	buttonsArea->setLayout(buttonsLayout);
	
	buttonsLayout->addItem(new QSpacerItem(1, 0, QSizePolicy::Expanding));

	selectButton = new QPushButton("Select", buttonsArea);
	cancelButton = new QPushButton("Cancel", buttonsArea);

	selectButton->setProperty("type", QVariant("primary"));
	selectButton->setEnabled(false);

	buttonsLayout->addWidget(selectButton);
	buttonsLayout->addWidget(cancelButton);

	layout->addWidget(classTree);
	layout->addWidget(buttonsArea);

	resize(500, 330);

	connect(classTree, &QTreeWidget::itemClicked, this, [=](QTreeWidgetItem* item, int) { selectButton->setEnabled(true); selectedClass = ((TTreeDataItem<FClass*>*)item)->GetData(); });
	connect(classTree, &QTreeWidget::itemDoubleClicked, this, [=](QTreeWidgetItem* item, int) { selectedClass = ((TTreeDataItem<FClass*>*)item)->GetData(); done(true); deleteLater(); });

	connect(selectButton, &QPushButton::clicked, this, [=](bool) { if (!selectedClass) return; done(true); deleteLater(); });
	connect(cancelButton, &QPushButton::clicked, this, [=](bool) { done(false); deleteLater(); });
}

CClassSelectorDialog::~CClassSelectorDialog()
{
}

int CClassSelectorDialog::exec()
{
	if (!filterClass)
		filterClass = CObject::StaticClass();

	// Generate tree
	{
		TTreeDataItem<FClass*>* item = new TTreeDataItem<FClass*>(filterClass, (QTreeWidget*)nullptr);
		item->setText(0, filterClass->GetName().c_str());
		classTree->insertTopLevelItem(0, item);

		TArray<FClass*> children;
		CModuleManager::FindChildClasses(filterClass, children);

		for (FClass* child : children)
			AddClassItem(child, item);
	}

	classTree->expandAll();

	return QDialog::exec();
}

void CClassSelectorDialog::AddClassItem(FClass* c, void* p)
{
	TTreeDataItem<FClass*>* parent = (TTreeDataItem<FClass*>*)p;
	TTreeDataItem<FClass*>* item = nullptr;

	if (!c->HasFlag(CTAG_HIDDEN))
	{
		item = new TTreeDataItem<FClass*>(c, parent);
		item->setText(0, c->GetName().c_str());
	}
	else
		item = parent;

	TArray<FClass*> children;
	CModuleManager::FindChildClasses(c, children);

	for (FClass* child : children)
		AddClassItem(child, item);
}
