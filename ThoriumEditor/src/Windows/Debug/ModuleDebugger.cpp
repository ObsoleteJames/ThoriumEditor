
#include "ModuleDebugger.h"
#include "Object/Class.h"
#include "Module.h"
#include "Widgets/CollapsableWidget.h"

#include <QToolBox>
#include <QSettings>
#include <QBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QTreeWidget>
#include <QSplitter>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QSpacerItem>

SDK_REGISTER_WINDOW(CModuleDebugger, "Module Viewer", "Debug", NULL);

enum ETreeModuleItemType
{
	MIT_MODULE,
	MIT_CLASS,
	MIT_STRUCT,
	MIT_ASSET,
	MIT_ENUM,
};

class CTreeModuleItem : public QTreeWidgetItem
{
	//Q_OBJECT

public:
	inline CTreeModuleItem(QTreeWidget* parent, int type) : QTreeWidgetItem(parent, type) {}
	inline CTreeModuleItem(QTreeWidgetItem* parent, int type) : QTreeWidgetItem(parent, type) {}

public:
	void* objPtr = nullptr;

};

bool CModuleDebugger::Shutdown()
{
	SaveState();
	return true;
}

void CModuleDebugger::SetupUi()
{
	CToolsWindow::SetupUi();

	setGeometry(QRect(0, 0, 640, 430));
	setWindowTitle("Module Viewer");

	Content = new QWidget(this);
	auto layout = new QVBoxLayout();
	Content->setLayout(layout);

	//Content->setStyleSheet("QWidget { background-color: #38393C; }");

	Properties = new QFrame(Content);
	propertiesLayout = new QVBoxLayout(Properties);
	Properties->setLayout(propertiesLayout);
	Properties->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
	propertiesLayout->setSpacing(0);

	PropertiesScroll = new QScrollArea(Content);
	PropertiesScroll->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	PropertiesScroll->setWidgetResizable(true);
	PropertiesScroll->setWidget(Properties);
	PropertiesScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	//PropertiesScroll->setStyleSheet("QWidget { background-color: #1a191a; color: #aaa; }");

	treeView = new QTreeWidget(Content);
	treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
	treeView->setColumnCount(2);
	treeView->setColumnWidth(0, 200);
	//treeView->setAlternatingRowColors(true);
	treeView->setHeaderLabels({ "Name", "Type" });

	connect(treeView, &QTreeWidget::itemSelectionChanged, this, [=]() { 
		if (treeView->selectedItems().size() == 0)
			return;

		auto* item = treeView->selectedItems()[0];
		UpdatePropertyList((CTreeModuleItem*)item); 
	});

	QSplitter* splitter = new QSplitter(Content);
	splitter->addWidget(treeView);
	splitter->addWidget(PropertiesScroll);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);

	layout->addWidget(splitter);
	setCentralWidget(Content);

	RestoreState();

	GenerateTree();
}

void CModuleDebugger::UpdatePropertyList(CTreeModuleItem* item)
{
	for (auto* w : curProperties)
	{
		propertiesLayout->removeWidget(w);
		w->deleteLater();
	}
	curProperties.Clear();
	
	if (item->type() == MIT_CLASS || item->type() == MIT_STRUCT || item->type() == MIT_ASSET)
	{
		FStruct* struc;
		FClass* clas = nullptr;
		FAssetClass* asset = nullptr;

		if (item->type() == MIT_ASSET)
		{
			asset = (FAssetClass*)item->objPtr;
			clas = static_cast<FClass*>(asset);
			struc = static_cast<FStruct*>(clas);
		}
		else if (item->type() == MIT_CLASS)
		{
			clas = (FClass*)item->objPtr;
			struc = static_cast<FStruct*>(clas);
		}
		else
			struc = (FStruct*)item->objPtr;

		// Name
		{
			QWidget* pWidget = new QWidget(Properties);
			QHBoxLayout* pLayout = new QHBoxLayout();
			pWidget->setLayout(pLayout);

			QLabel* pLabel = new QLabel("Name: ", pWidget);
			pLabel->setMinimumSize({ 100, 0 });

			QLineEdit* pEdit = new QLineEdit(pWidget);
			pEdit->setReadOnly(true);
			pEdit->setText(struc->GetName().c_str());

			pLayout->addWidget(pLabel);
			pLayout->addWidget(pEdit);

			propertiesLayout->addWidget(pWidget);
			curProperties.Add(pWidget);
		}
		// Internal Name
		{
			QWidget* pWidget = new QWidget(Properties);
			QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
			pWidget->setLayout(pLayout);

			QLabel* pLabel = new QLabel("Internal Name: ", pWidget);
			pLabel->setMinimumSize({ 100, 0 });

			QLineEdit* pEdit = new QLineEdit(pWidget);
			pEdit->setReadOnly(true);
			pEdit->setText(struc->GetInternalName().c_str());

			pLayout->addWidget(pLabel);
			pLayout->addWidget(pEdit);

			propertiesLayout->addWidget(pWidget);
			curProperties.Add(pWidget);
		}
		// Description
		{
			QWidget* pWidget = new QWidget(Properties);
			QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
			pWidget->setLayout(pLayout);

			QLabel* pLabel = new QLabel("Description: ", pWidget);
			pLabel->setMinimumSize({ 100, 0 });

			QLineEdit* pEdit = new QLineEdit(pWidget);
			pEdit->setReadOnly(true);
			pEdit->setText(struc->GetDescription().c_str());

			pLayout->addWidget(pLabel);
			pLayout->addWidget(pEdit);

			propertiesLayout->addWidget(pWidget);
			curProperties.Add(pWidget);
		}
		// Size
		{
			QWidget* pWidget = new QWidget(Properties);
			QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
			pWidget->setLayout(pLayout);

			QLabel* pLabel = new QLabel("Size: ", pWidget);
			pLabel->setMinimumSize({ 100, 0 });

			QSpinBox* pEdit = new QSpinBox(pWidget);
			pEdit->setMinimum(INT32_MIN);
			pEdit->setMaximum(INT32_MAX);
			pEdit->setReadOnly(true);
			pEdit->setValue(struc->Size());

			pLayout->addWidget(pLabel);
			pLayout->addWidget(pEdit);

			propertiesLayout->addWidget(pWidget);
			curProperties.Add(pWidget);
		}
		// Base Class
		if (struc->IsClass() && clas->GetBaseClass())
		{
			QWidget* pWidget = new QWidget(Properties);
			QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
			pWidget->setLayout(pLayout);

			QLabel* pLabel = new QLabel("Base Class: ", pWidget);
			pLabel->setMinimumSize({ 100, 0 });

			QLineEdit* pEdit = new QLineEdit(pWidget);
			pEdit->setReadOnly(true);
			pEdit->setText(clas->GetBaseClass()->cppName.c_str());

			pLayout->addWidget(pLabel);
			pLayout->addWidget(pEdit);

			propertiesLayout->addWidget(pWidget);
			curProperties.Add(pWidget);
		}
		// Asset Extension
		/*if (asset)
		{
			QWidget* pWidget = new QWidget(Properties);
			QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
			pWidget->setLayout(pLayout);

			QLabel* pLabel = new QLabel("Extension: ", pWidget);
			pLabel->setMinimumSize({ 100, 0 });

			QLineEdit* pEdit = new QLineEdit(pWidget);
			pEdit->setReadOnly(true);
			pEdit->setText(asset->GetExtension().c_str());

			pLayout->addWidget(pLabel);
			pLayout->addWidget(pEdit);

			propertiesLayout->addWidget(pWidget);
			curProperties.Add(pWidget);
		}*/

		if (struc->GetPropertyList())
		{
			QLabel* label = new QLabel("Properties:", Properties);
			label->setMinimumSize({ 0, 30 });
			propertiesLayout->addWidget(label);

			QFrame* line = new QFrame(Properties);
			line->setFrameShadow(QFrame::Sunken);
			line->setLineWidth(2);
			line->setFrameShape(QFrame::HLine);

			propertiesLayout->addWidget(line);
			curProperties.Add(label);
			curProperties.Add(line);
		}

		for (const FProperty* p = struc->GetPropertyList(); p != nullptr; p = p->next)
		{
			CCollapsableWidget* pWidget = new CCollapsableWidget(p->name.c_str(), nullptr, Properties);

			QFrame* pContent = new QFrame(pWidget);
			pContent->setProperty("type", QVariant(2));
			QVBoxLayout* pLayout = new QVBoxLayout(pContent);

			// Type
			{
				QWidget* iWidget = new QWidget(pContent);
				QHBoxLayout* iLayout = new QHBoxLayout(iWidget);
				
				QLabel* label = new QLabel("Type: ", iWidget);
				QComboBox* comboBox = new QComboBox(iWidget);
				comboBox->setMinimumWidth(140);

				comboBox->addItem("STRUCT");
				comboBox->addItem("CLASS");
				comboBox->addItem("STRING");
				comboBox->addItem("ENUM");
				comboBox->addItem("ARRAY");
				comboBox->addItem("OBJECT PTR");
				comboBox->addItem("CLASS PTR");
				comboBox->addItem("FLOAT");
				comboBox->addItem("DOUBLE");
				comboBox->addItem("INT");
				comboBox->addItem("UINT");
				comboBox->addItem("BOOL");

				comboBox->setCurrentIndex(p->type);
				comboBox->setEnabled(false);
				
				iLayout->addWidget(label);

				if (p->type == EVT_ARRAY || p->type == EVT_OBJECT_PTR || p->type == EVT_CLASS_PTR)
				{
					iLayout->addSpacerItem(new QSpacerItem(1, 0, QSizePolicy::Expanding));
					QLabel* typeName = new QLabel((FString("Typename: ") + p->typeName).c_str(), iWidget);
					iLayout->addWidget(typeName);
				}
				
				iLayout->setSpacing(16);
				iLayout->addWidget(comboBox);
				pLayout->addWidget(iWidget);
			}
			// Offset
			{
				QWidget* iWidget = new QWidget(pContent);
				QHBoxLayout* iLayout = new QHBoxLayout(iWidget);

				QLabel* label = new QLabel("Offset: ", iWidget);
				QSpinBox* value = new QSpinBox(iWidget);

				value->setMinimum(INT32_MIN);
				value->setMaximum(INT32_MAX);
				value->setValue(p->offset);
				value->setReadOnly(true);

				iLayout->addWidget(label);
				iLayout->addWidget(value);
				pLayout->addWidget(iWidget);
			}
			// Size
			{
				QWidget* iWidget = new QWidget(pContent);
				QHBoxLayout* iLayout = new QHBoxLayout(iWidget);

				QLabel* label = new QLabel("Size: ", iWidget);
				QSpinBox* value = new QSpinBox(iWidget);

				value->setMinimum(INT32_MIN);
				value->setMaximum(INT32_MAX);
				value->setValue(p->size);
				value->setReadOnly(true);

				iLayout->addWidget(label);
				iLayout->addWidget(value);
				pLayout->addWidget(iWidget);
			}

			if (p->meta)
			{
				CCollapsableWidget* metaWidget = new CCollapsableWidget("MetaData", nullptr, pContent);
				metaWidget->SetHeaderType(CCollapsableWidget::NESTED_HEADER);
				pLayout->addWidget(metaWidget);

				QWidget* metaContent = new QWidget(metaWidget);
				metaWidget->SetWidget(metaContent);
				QVBoxLayout* metaLayout = new QVBoxLayout(metaContent);

				auto* meta = p->meta;
				for (int i = 0; i < meta->numGenericFlags; i++)
				{
					auto* flag = &meta->genericFlags[i];
					QHBoxLayout* flagLayout = new QHBoxLayout();
					QLabel* flagLabel = new QLabel((flag->Key).c_str(), metaContent);
					QLabel* flagValue = new QLabel((flag->Value).c_str(), metaContent);

					flagLayout->addWidget(flagLabel);
					flagLayout->addWidget(flagValue);
					metaLayout->addLayout(flagLayout);
				}
			}

			pWidget->SetWidget(pContent);
			pWidget->SetCollapsed(true);

			propertiesLayout->addWidget(pWidget);
			curProperties.Add(pWidget);
		}

		if (clas)
		{
			if (clas->GetFunctionList())
			{
				QLabel* label = new QLabel("Functions:", Properties);
				label->setMinimumSize({ 0, 30 });
				propertiesLayout->addWidget(label);

				QFrame* line = new QFrame(Properties);
				line->setFrameShadow(QFrame::Sunken);
				line->setLineWidth(2);
				line->setFrameShape(QFrame::HLine);

				propertiesLayout->addWidget(line);
				curProperties.Add(label);
				curProperties.Add(line);
			}

			for (const FFunction* func = clas->GetFunctionList(); func != nullptr; func = func->next)
			{
				CCollapsableWidget* pWidget = new CCollapsableWidget(func->name.c_str(), nullptr, Properties);

				QFrame* pContent = new QFrame(pWidget);
				pContent->setProperty("type", QVariant(2));
				QVBoxLayout* pLayout = new QVBoxLayout(pContent);

				{
					QHBoxLayout* typeLayout = new QHBoxLayout();
					QLabel* typeLabel = new QLabel("Function Type: ", pContent);
					
					const char* types[] = {
						"GENERAL",
						"OUTPUT",
						"COMMAND",
						"SERVER_RPC",
						"CLIENT_RPC",
						"MULTICAST_RPC",
					};
					QLabel* typeValue = new QLabel(types[func->type], pContent);

					typeLayout->addWidget(typeLabel);
					typeLayout->addWidget(typeValue);
					pLayout->addLayout(typeLayout);
				}

				for (int i = 0; i < func->numArguments; i++)
				{
					const auto& arg = func->Arguments[i];
					QHBoxLayout* argLayout = new QHBoxLayout();
					QLabel* argLabel = new QLabel((FString("Arg ") + FString::ToString(i) + ": " + arg.name).c_str(), pContent);

					const char* types[] = {
						"STRUCT",
						"CLASS",
						"STRING",
						"ENUM",
						"ARRAY",
						"OBJECT PTR",
						"CLASS PTR",
						"FLOAT",
						"DOUBLE",
						"INT",
						"UINT",
						"BOOL",
					};
					QLabel* argType = new QLabel((FString("Type: ") + types[arg.type.type]).c_str(), pContent);

					argLayout->addWidget(argLabel);
					argLayout->addWidget(argType);
					pLayout->addLayout(argLayout);
				}

				pWidget->SetWidget(pContent);
				pWidget->SetCollapsed(true);

				propertiesLayout->addWidget(pWidget);
				curProperties.Add(pWidget);
			}
		}
	}
	else if (item->type() == MIT_ENUM)
	{
		FEnum* eNum = (FEnum*)item->objPtr;

		// Name
		{
			QWidget* pWidget = new QWidget(Properties);
			QHBoxLayout* pLayout = new QHBoxLayout();
			pWidget->setLayout(pLayout);

			QLabel* pLabel = new QLabel("Name: ", pWidget);
			pLabel->setMinimumSize({ 100, 0 });

			QLineEdit* pEdit = new QLineEdit(pWidget);
			pEdit->setReadOnly(true);
			pEdit->setText(eNum->GetName().c_str());

			pLayout->addWidget(pLabel);
			pLayout->addWidget(pEdit);

			propertiesLayout->addWidget(pWidget);
			curProperties.Add(pWidget);
		}
		// Internal Name
		{
			QWidget* pWidget = new QWidget(Properties);
			QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
			pWidget->setLayout(pLayout);

			QLabel* pLabel = new QLabel("Internal Name: ", pWidget);
			pLabel->setMinimumSize({ 100, 0 });

			QLineEdit* pEdit = new QLineEdit(pWidget);
			pEdit->setReadOnly(true);
			pEdit->setText(eNum->GetInternalName().c_str());

			pLayout->addWidget(pLabel);
			pLayout->addWidget(pEdit);

			propertiesLayout->addWidget(pWidget);
			curProperties.Add(pWidget);
		}
		// Description
		{
			QWidget* pWidget = new QWidget(Properties);
			QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
			pWidget->setLayout(pLayout);

			QLabel* pLabel = new QLabel("Description: ", pWidget);
			pLabel->setMinimumSize({ 100, 0 });

			QLineEdit* pEdit = new QLineEdit(pWidget);
			pEdit->setReadOnly(true);
			pEdit->setText(eNum->GetDescription().c_str());

			pLayout->addWidget(pLabel);
			pLayout->addWidget(pEdit);

			propertiesLayout->addWidget(pWidget);
			curProperties.Add(pWidget);
		}
		// Size
		{
			QWidget* pWidget = new QWidget(Properties);
			QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
			pWidget->setLayout(pLayout);

			QLabel* pLabel = new QLabel("Size: ", pWidget);
			pLabel->setMinimumSize({ 100, 0 });

			QSpinBox* pEdit = new QSpinBox(pWidget);
			pEdit->setMinimum(INT32_MIN);
			pEdit->setMaximum(INT32_MAX);
			pEdit->setReadOnly(true);
			pEdit->setValue(eNum->Size());

			pLayout->addWidget(pLabel);
			pLayout->addWidget(pEdit);

			propertiesLayout->addWidget(pWidget);
			curProperties.Add(pWidget);
		}

		{
			QLabel* label = new QLabel("Values:", Properties);
			label->setMinimumSize({ 0, 30 });
			propertiesLayout->addWidget(label);

			QFrame* line = new QFrame(Properties);
			line->setFrameShadow(QFrame::Sunken);
			line->setLineWidth(2);
			line->setFrameShape(QFrame::HLine);

			propertiesLayout->addWidget(line);
			curProperties.Add(label);
			curProperties.Add(line);
		}

		auto& values = eNum->GetValues();

		for (SizeType i = 0; i < values.Size(); i++)
		{
			QWidget* pWidget = new QWidget(Properties);
			QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
			pWidget->setLayout(pLayout);

			QLabel* pLabel = new QLabel(QString::number(i) + ": ", pWidget);
			pLabel->setMinimumSize({ 100, 0 });

			QLabel* pLabel2 = new QLabel((values[i].Key + " = " + FString::ToString(values[i].Value)).c_str(), pWidget);

			pLayout->addWidget(pLabel);
			pLayout->addWidget(pLabel2);

			propertiesLayout->addWidget(pWidget);
			curProperties.Add(pWidget);
		}
	}

	//propertiesLayout->addSpacerItem(new QSpacerItem(0, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void CModuleDebugger::GenerateTree()
{
	TArray<CModule*> ModulesList = CModuleManager::GetModules();

	// Delete removed modules
	for (SizeType i = 0; i < treeView->invisibleRootItem()->childCount(); i++)
	{
		CTreeModuleItem* it = (CTreeModuleItem*)treeView->invisibleRootItem()->child(i);
		bool bExists = false;
		for (auto _module : ModulesList)
		{
			if (_module == it->objPtr)
			{
				bExists = true;
				break;
			}
		}

		if (!bExists)
			treeView->invisibleRootItem()->removeChild(it);
	}

	for (auto _module : ModulesList)
	{
		CTreeModuleItem* moduleItem = nullptr;
		for (SizeType i = 0; i < treeView->invisibleRootItem()->childCount(); i++)
		{
			CTreeModuleItem* it = (CTreeModuleItem*)treeView->invisibleRootItem()->child(i);
			if (_module == it->objPtr)
			{
				moduleItem = it;
				break;
			}
		}

		if (!moduleItem)
		{
			moduleItem = new CTreeModuleItem(treeView, MIT_MODULE);
			moduleItem->objPtr = _module;
		}

		moduleItem->setText(0, _module->Name().c_str());
		moduleItem->setText(1, "CModule");

		for (auto _class : _module->Classes)
		{
			auto* item = new CTreeModuleItem(moduleItem, MIT_CLASS);
			item->objPtr = _class;

			item->setText(0, _class->GetName().c_str());
			item->setText(1, "FClass");
		}
		for (auto _class : _module->Structures)
		{
			auto* item = new CTreeModuleItem(moduleItem, MIT_STRUCT);
			item->objPtr = _class;

			item->setText(0, _class->GetName().c_str());
			item->setText(1, "FStruct");
		}
		for (auto _class : _module->Enums)
		{
			auto* item = new CTreeModuleItem(moduleItem, MIT_ENUM);
			item->objPtr = _class;

			item->setText(0, _class->GetName().c_str());
			item->setText(1, "FEnum");
		}
		for (auto _class : _module->Assets)
		{
			auto* item = new CTreeModuleItem(moduleItem, MIT_ASSET);
			item->objPtr = _class;

			item->setText(0, _class->GetName().c_str());
			item->setText(1, "FAssetClass");
		}
	}
}

void CModuleDebugger::UserSaveState(QSettings& settings)
{
	settings.setValue("treeGeo", treeView->saveGeometry());
}

void CModuleDebugger::UserRestoreState(QSettings& settings)
{
	treeView->restoreGeometry(settings.value("treeGeo").toByteArray());
}
