#include "AssetManagerWnd.h"
#include "Assets/Asset.h"
#include "Assets/AssetManager.h"
#include "Assets/ModelAsset.h"
#include "Assets/Material.h"
#include "Assets/Scene.h"

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
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>

SDK_REGISTER_WINDOW(CAssetManagerWnd, "Asset Manager", "Debug", NULL);

bool CAssetManagerWnd::Shutdown()
{
	SaveState();
    return true;
}

static QString FStringToQString(const FString& s)
{
	return QString::fromUtf8((const char*)s.Data());
}

static void ClearLayoutRecursive(QLayout* layout)
{
	if (!layout) return;
	QLayoutItem* item = nullptr;
	while ((item = layout->takeAt(0)) != nullptr)
	{
		if (auto w = item->widget())
		{
			w->deleteLater();
		}
		if (auto l = item->layout())
		{
			ClearLayoutRecursive(l);
			delete l;
		}
		delete item;
	}
}

static void AddLabelPair(QVBoxLayout* layout, QWidget* parent, const QString& name, const QString& value)
{
	auto w = new QWidget(parent);
	auto l = new QHBoxLayout(w);
	l->setContentsMargins(0,0,0,0);
	l->setSpacing(6);
	l->addWidget(new QLabel(name + ": ", w));
	auto v = new QLineEdit(value, w);
	v->setReadOnly(true);
	v->setFrame(false);
	l->addWidget(v);
	layout->addWidget(w);
}

static void PopulateTree(QTreeWidget* treeView)
{
	if (!treeView) return;
	treeView->clear();

	const auto& assets = CAssetManager::GetAssetsData();
	for (auto it = assets.begin(); it != assets.end(); ++it)
	{
		SizeType id = it->first;
		const FAssetData& ad = it->second;

		QString name;
		if (ad.file)
			name = FStringToQString(ad.file->Name());
		else
			name = QString("asset_%1").arg((qulonglong)id);

		QString type;
		if (ad.type)
			type = ad.type->GetName().c_str();
		else
			type = "Generic";

		auto item = new QTreeWidgetItem(treeView);
		item->setText(0, name);
		item->setText(1, type);
		item->setData(0, Qt::UserRole, QVariant::fromValue<qulonglong>((qulonglong)id));
		treeView->addTopLevelItem(item);
	}
}

static void ShowAssetHeader(const FAssetData* assetData, QVBoxLayout* propertiesLayout, QFrame* parent)
{
	if (!assetData || !propertiesLayout) return;

	QString name = assetData->file ? FStringToQString(assetData->file->Path()) : QString("asset_%1").arg((qulonglong)assetData->id);
	propertiesLayout->addWidget(new QLabel(QString("<b>%1</b>").arg(name), parent));
	QString type = assetData->type ? QString("type: %1").arg(assetData->type->GetName().c_str()) : "Generic";
	propertiesLayout->addWidget(new QLabel(type, parent));
	propertiesLayout->addSpacing(6);
}

static void ShowAssetBasicInfo(const FAssetData* assetData, QVBoxLayout* propertiesLayout, QFrame* parent)
{
	if (!assetData || !propertiesLayout) return;

	auto infoWidget = new QWidget(parent);
	auto infoLayout = new QVBoxLayout(infoWidget);
	infoLayout->setContentsMargins(0,0,0,0);
	infoWidget->setLayout(infoLayout);

	infoLayout->addWidget(new QLabel(QString("AssetId: %1").arg((qulonglong)assetData->id), infoWidget));

	// File path / size / extension
	if (assetData->file)
	{
		infoLayout->addWidget(new QLabel(QString("Path: %1").arg(FStringToQString(assetData->file->Path())), infoWidget));
		infoLayout->addWidget(new QLabel(QString("Extension: %1").arg(FStringToQString(assetData->file->Extension())), infoWidget));
		infoLayout->addWidget(new QLabel(QString("Size: %1 bytes").arg((qulonglong)assetData->file->Size()), infoWidget));
	}

	// Version info from FAssetData
	infoLayout->addWidget(new QLabel(QString("Asset version: %1").arg((int)assetData->version), infoWidget));

	// Loaded check via CAssetManager
	bool loaded = CAssetManager::IsAssetLoaded(assetData->id);
	infoLayout->addWidget(new QLabel(QString("Loaded: %1").arg(loaded ? "Yes" : "No"), infoWidget));

	propertiesLayout->addWidget(infoWidget);
}

static void InspectAssetInstance(CAsset* asset, QVBoxLayout* propertiesLayout, QFrame* parentFrame)
{
	if (!asset || !propertiesLayout) return;

	// Header (path/class already shown by callers; show runtime class and CAsset fields)
	{
		FClass* cls = asset->GetClass();
		QString className = cls ? QString::fromUtf8(cls->cppName.c_str()) : QString("UnknownClass");
		propertiesLayout->addWidget(new QLabel(QString("<i>%1</i>").arg(className), parentFrame));
	}

	// Basic CAsset fields
	AddLabelPair(propertiesLayout, parentFrame, "AssetId", QString::number((qulonglong)asset->AssetId()));
	AddLabelPair(propertiesLayout, parentFrame, "Version", QString::number((int)asset->Version()));
	AddLabelPair(propertiesLayout, parentFrame, "AssetVersion", QString::number((int)asset->AssetVersion()));
	AddLabelPair(propertiesLayout, parentFrame, "IsDirty", asset->IsDirty() ? "Yes" : "No");
	AddLabelPair(propertiesLayout, parentFrame, "LOD0 Loaded", asset->IsLodLoaded(0) ? "Yes" : "No");

	// Model-specific inspection
	if (auto* model = Cast<CModelAsset>(asset))
	{
		AddLabelPair(propertiesLayout, parentFrame, "Mesh count", QString::number(model->GetMeshes().Size()));
		AddLabelPair(propertiesLayout, parentFrame, "Material count", QString::number(model->GetMaterials().Size()));
		AddLabelPair(propertiesLayout, parentFrame, "LOD count", QString::number((int)model->LodCount()));

		// Mesh names (model provides meshNames in header; use accessor if available)
		{
			const auto& mn = model->GetMeshNames();
			if (mn.Size() > 0)
			{
				propertiesLayout->addWidget(new QLabel("Meshes:", parentFrame));
				for (SizeType i = 0; i < mn.Size(); ++i)
					propertiesLayout->addWidget(new QLabel(QString("  %1: %2").arg((qulonglong)i).arg(FStringToQString(mn[i])), parentFrame));
			}
			else if (model->GetMeshes().Size() > 0)
			{
				propertiesLayout->addWidget(new QLabel("Meshes (index):", parentFrame));
				for (SizeType i = 0; i < model->GetMeshes().Size(); ++i)
					propertiesLayout->addWidget(new QLabel(QString("  Mesh %1").arg((qulonglong)i), parentFrame));
			}
		}

		// Materials
		{
			const auto& mats = model->GetMaterials();
			if (mats.Size() > 0)
			{
				propertiesLayout->addWidget(new QLabel("Materials:", parentFrame));
				for (SizeType i = 0; i < mats.Size(); ++i)
				{
					const FMaterial& m = mats[i];
					QString matLine = QString("  %1").arg(FStringToQString(m.name.IsEmpty() ? m.path : m.name));
					if (m.obj.IsValid())
						matLine += QString(" (loaded: %1)").arg(FStringToQString(m.obj->GetPath()));
					else if (!m.path.IsEmpty())
						matLine += QString(" (path: %1)").arg(FStringToQString(m.path));
					propertiesLayout->addWidget(new QLabel(matLine, parentFrame));
				}
			}
		}

		// LOD groups
		{
			propertiesLayout->addWidget(new QLabel("LOD Groups:", parentFrame));
			for (uint8 i = 0; i < model->LodCount(); ++i)
			{
				const FLODGroup& g = model->GetLODs()[i];
				QString s = QString("  LOD %1 - meshes: %2, distanceBias: %3").arg(i).arg((qulonglong)g.meshIndices.Size()).arg(g.distanceBias);
				propertiesLayout->addWidget(new QLabel(s, parentFrame));
			}
		}

		// Body groups
		{
			const auto& bgs = model->GetBodyGroups();
			if (bgs.Size() > 0)
			{
				propertiesLayout->addWidget(new QLabel("BodyGroups:", parentFrame));
				for (SizeType i = 0; i < bgs.Size(); ++i)
				{
					const FBodyGroup& bg = bgs[i];
					QString s = QString("  %1 - options: %2").arg(FStringToQString(bg.name)).arg((qulonglong)bg.options.Size());
					propertiesLayout->addWidget(new QLabel(s, parentFrame));
					for (SizeType o = 0; o < bg.options.Size(); ++o)
					{
						const FBodyGroupOption& opt = bg.options[o];
						QString os = QString("    Option %1: %2 meshes").arg((qulonglong)o).arg((qulonglong)opt.meshIndices.Size());
						propertiesLayout->addWidget(new QLabel(os, parentFrame));
					}
				}
			}
		}

		// Colliders
		{
			const auto& coll = model->GetColliders();
			if (coll.Size() > 0)
			{
				propertiesLayout->addWidget(new QLabel("Colliders:", parentFrame));
				for (SizeType i = 0; i < coll.Size(); ++i)
				{
					const FModelCollider& c = coll[i];
					QString s = QString("  %1 - shapeType=%2 meshIndex=%3 attachBone=%4 complex=%5")
						.arg((qulonglong)i)
						.arg((int)c.shapeType)
						.arg((qulonglong)c.meshIndex)
						.arg((qulonglong)c.attachBone)
						.arg(c.bComplex ? "Yes" : "No");
					propertiesLayout->addWidget(new QLabel(s, parentFrame));
				}
			}
		}

		// Bounds
		{
			auto b = model->GetBounds();
			QString s = QString("Bounds center(%1,%2,%3) extents(%4,%5,%6)")
				.arg(b.position.x).arg(b.position.y).arg(b.position.z)
				.arg(b.extents.x).arg(b.extents.y).arg(b.extents.z);
			propertiesLayout->addWidget(new QLabel(s, parentFrame));
		}
	}

	// Material-specific inspection
	if (auto* mat = Cast<CMaterial>(asset))
	{
		if (mat->GetShaderSource())
			AddLabelPair(propertiesLayout, parentFrame, "Shader", QString::fromUtf8(mat->GetShaderSource()->Name().c_str()));
		AddLabelPair(propertiesLayout, parentFrame, "Preferred Render Pass", QString::number(mat->preferredRenderPass));
		AddLabelPair(propertiesLayout, parentFrame, "DoDepthTest", mat->DoDepthTest() ? "Yes" : "No");

		const auto& texs = mat->GetTextures();
		propertiesLayout->addWidget(new QLabel(QString("Textures: %1").arg((qulonglong)texs.Size()), parentFrame));
		for (SizeType i = 0; i < texs.Size(); ++i)
		{
			const auto& t = texs[i];
			QString line = QString("  %1 (reg %2)").arg(FStringToQString(t.name)).arg((int)t.registerId);
			if (t.tex.IsValid())
				line += QString(" -> %1").arg(FStringToQString(t.tex->GetPath()));
			else if (t.bIsCustom)
				line += QString(" (custom color: %1,%2,%3,%4)").arg(t.color[0]).arg(t.color[1]).arg(t.color[2]).arg(t.color[3]);
			propertiesLayout->addWidget(new QLabel(line, parentFrame));
		}
	}

	// Scene-specific inspection
	if (auto* scene = Cast<CScene>(asset))
	{
		/*AddLabelPair(propertiesLayout, parentFrame, "Gravity", QString::number(scene->gravity));
		if (scene->gamemodeClass.Get())
		{
			FClass* gm = scene->gamemodeClass.Get();
			if (gm)
				AddLabelPair(propertiesLayout, parentFrame, "GameMode", QString::fromUtf8(gm->cppName.c_str()));
		}*/
	}
}

void CAssetManagerWnd::SetupUi()
{
	CToolsWindow::SetupUi();

	setGeometry(QRect(0, 0, 640, 430));
	setWindowTitle("Asset Manager");

	Content = new QWidget(this);
	auto layout = new QVBoxLayout();
	// reduce default spacing/margins so toolbar doesn't create a large gap
	layout->setSpacing(4);
	layout->setContentsMargins(4, 4, 4, 4);
	Content->setLayout(layout);

	// --- Toolbar with Refresh ---
	{
		auto toolbar = new QWidget(Content);

		// Toolbar should not expand vertically and should be compact.
		toolbar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
		toolbar->setMaximumHeight(36);

		auto tbLayout = new QHBoxLayout();
		tbLayout->setContentsMargins(4, 4, 4, 4);
		tbLayout->setSpacing(6);
		toolbar->setLayout(tbLayout);

		auto btnRefresh = new QPushButton("Refresh", toolbar);
		// Prevent the button from expanding to fill available horizontal space.
		btnRefresh->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
		btnRefresh->setMaximumWidth(140);
		btnRefresh->setFixedHeight(24);

		tbLayout->addWidget(btnRefresh);
		tbLayout->addStretch(1);

		layout->addWidget(toolbar);

		connect(btnRefresh, &QPushButton::clicked, [this]() {
			PopulateTree(treeView);
		});
	}

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

	treeView = new QTreeWidget(Content);
	treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
	treeView->setColumnCount(2);
	treeView->setColumnWidth(0, 300);
	treeView->setHeaderLabels({ "Name", "Type" });
	treeView->header()->setSectionResizeMode(0, QHeaderView::Interactive);
	treeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);

	splitter = new QSplitter(Content);
	splitter->addWidget(treeView);
	splitter->addWidget(PropertiesScroll);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);

	layout->addWidget(splitter);
	setCentralWidget(Content);

	// Populate initially
	PopulateTree(treeView);

	// Selection handling: inspect selected asset
	connect(treeView, &QTreeWidget::itemSelectionChanged, [this]() {
		// Clear previous properties
		ClearLayoutRecursive(propertiesLayout);

		auto items = treeView->selectedItems();
		if (items.isEmpty()) return;

		auto item = items.first();
		bool ok = false;
		qulonglong idVal = item->data(0, Qt::UserRole).toULongLong(&ok);
		if (!ok) return;
		SizeType assetId = (SizeType)idVal;

		const FAssetData* assetData = CAssetManager::GetAssetData(assetId);
		if (!assetData)
		{
			propertiesLayout->addWidget(new QLabel("Asset data unavailable", Properties));
			return;
		}

		// Header + basic info
		ShowAssetHeader(assetData, propertiesLayout, Properties);
		ShowAssetBasicInfo(assetData, propertiesLayout, Properties);

		// If an instance exists and loaded, fetch it and inspect directly (raw fields / type-specific)
		bool loaded = CAssetManager::IsAssetLoaded(assetData->id);
		if (loaded)
		{
			TObjectPtr<CAsset> instance = CAssetManager::GetAsset(assetData->type, assetData->id);
			if (instance.IsValid())
			{
				InspectAssetInstance(instance, propertiesLayout, Properties);
			}
			else
			{
				propertiesLayout->addWidget(new QLabel("Asset instance not created (not allocated).", Properties));
			}
		}
		else
		{
			propertiesLayout->addWidget(new QLabel("Asset is not loaded; load it to inspect instance data.", Properties));
		}

		propertiesLayout->addStretch(1);
	});
}

void CAssetManagerWnd::UserSaveState(QSettings& settings)
{
	settings.setValue("tree_widget", treeView->columnWidth(0));
	settings.setValue("splitter_widget", splitter->saveState());
}

void CAssetManagerWnd::UserRestoreState(QSettings& settings)
{
	int w0 = settings.value("tree_col0_width", 300).toInt();
	treeView->setColumnWidth(0, 0);
	splitter->restoreState(settings.value("splitter_widget").toByteArray());
}
