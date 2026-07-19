
#include "Widgets/ContentBrowser.h"
#include "Assets/TextureAsset.h"
#include "Console.h"

#include <QDialog>
#include <QScrollArea>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>

struct FImportSettings
{
	int assetType = 0; // 0: Texture2D, 1: TextureCube
	bool bSrgb = true;
	uint8 numMipMaps = 8;
	ETextureAssetFormat format = THTX_FORMAT_AUTO_COMPRESSED;
	ETextureFilter filter = THTX_FILTER_ANISOTROPIC;
};

class FTextureImportAction : public FAssetImportAction
{
public:
	FTextureImportAction()
	{
		targetClass = (FAssetClass*)CTexture::StaticClass();
		importableTypes = ".png;.jpg;.tga;.bmp;.hdr";
	}

	void Invoke(FBrowserActionData* d) override
	{
		FBAImportFile* data = (FBAImportFile*)d;

		bool bImportAll = false;
		FImportSettings settings{};
		for (auto& file : data->sourceFiles)
		{
			if (!bImportAll)
			{
				QDialog dialog(d->browser);
				dialog.setWindowTitle("Texture Import Options");

				QGridLayout* layout = new QGridLayout(&dialog);

				layout->addWidget(new QLabel(("File: " + file).c_str(), &dialog), 0, 0, 1, 4);

				QScrollArea* scroll = new QScrollArea(&dialog);
				scroll->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
				scroll->setWidgetResizable(true);
				scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
				layout->addWidget(scroll, 1, 0, 1, 4);

				QWidget* page = new QWidget(&dialog);
				QGridLayout* playout = new QGridLayout(page);
				playout->getContentsMargins(0, 0, 0, 0);
				page->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
				scroll->setWidget(page);

				playout->addWidget(new QLabel("Asset Type", page), 0, 0);

				QComboBox* comboType = new QComboBox(page);
				comboType->addItem("Texture 2D");
				comboType->addItem("Cubemap");
				comboType->setCurrentIndex(settings.assetType);
				playout->addWidget(comboType, 0, 1);

				playout->addWidget(new QLabel("sRGB", page), 1, 0);
				
				QCheckBox* checkSRGB = new QCheckBox(page);
				checkSRGB->setChecked(settings.bSrgb);
				playout->addWidget(checkSRGB, 1, 1);

				playout->addWidget(new QLabel("Mipmap Count", page), 2, 0);

				QSpinBox* editMipmap = new QSpinBox(page);
				editMipmap->setMinimum(0);
				editMipmap->setMaximum(8);
				editMipmap->setValue(settings.numMipMaps);
				playout->addWidget(editMipmap, 2, 1);

				playout->addWidget(new QLabel("Format", page), 3, 0);

				QComboBox* comboFormat = new QComboBox(page);
				comboFormat->addItem("R8 Uint");
				comboFormat->addItem("RG8 Uint");
				comboFormat->addItem("RGB8 Uint");
				comboFormat->addItem("RGBA8 Uint");
				comboFormat->addItem("RGBA16 Float");
				comboFormat->addItem("RGBA32 Float");
				comboFormat->addItem("DXT1");
				comboFormat->addItem("DXT5");
				comboFormat->addItem("Auto");
				comboFormat->addItem("Auto Compressed");
				comboFormat->setCurrentIndex(settings.format);
				playout->addWidget(comboFormat, 3, 1);

				playout->addWidget(new QLabel("Filtering", page), 4, 0);

				QComboBox* comboFilter = new QComboBox(page);
				comboFilter->addItem("Linear");
				comboFilter->addItem("Point");
				comboFilter->addItem("Anisotropic");
				comboFilter->setCurrentIndex(settings.filter);
				playout->addWidget(comboFilter, 4, 1);

				QPushButton* btnImportAll = new QPushButton("Import All", &dialog);
				QPushButton* btnImport = new QPushButton("Import", &dialog);
				QPushButton* btnCancel = new QPushButton("Cancel", &dialog);

				layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 2, 0);
				layout->addWidget(btnImportAll, 2, 1);
				layout->addWidget(btnImport, 2, 2);
				layout->addWidget(btnCancel, 2, 3);
				
				QDialog* d = &dialog;
				QObject::connect(btnImportAll, &QPushButton::clicked, &dialog, [=]() { d->done(2); });
				QObject::connect(btnImport, &QPushButton::clicked, &dialog, [=]() { d->done(1); });
				QObject::connect(btnCancel, &QPushButton::clicked, &dialog, [=]() { d->done(0); });

				int r = dialog.exec();

				if (r == 0)
					return;
				if (r == 2)
					bImportAll = true;

				settings.assetType = comboType->currentIndex();
				settings.bSrgb = checkSRGB->isChecked();
				settings.numMipMaps = editMipmap->value();
				settings.format = (ETextureAssetFormat)comboFilter->currentIndex();
				settings.filter = (ETextureFilter)comboFilter->currentIndex();
			}

			if (settings.assetType == 0)
			{
				FString path = data->outPath;
				FString name = file;
				if (auto i = name.FindLastOf("\\/"); i != -1)
					name.Erase(name.begin(), name.begin() + i + 1);
				if (auto i = name.FindLastOf('.'); i != -1)
					name.Erase(name.begin() + i, name.end());
				
				path += "/" + name;

				// check if the texture already exists. this will overwrite the existing file.
				TObjectPtr<CTexture> tex = CAssetManager::GetAsset<CTexture>(path);

				// create a new texture if it doesn't already exist or if it's not in the same mod.
				if (!tex || tex->File()->Mod()->Name() != data->outMod)
				{ 
					tex = CreateObject<CTexture>();
					THORIUM_ASSERT(CAssetManager::RegisterNewAsset(tex, path, data->outMod), "Failed to register CTexture asset!");
				}

				if (!tex->Import(ToFString(file), { settings.numMipMaps, settings.bSrgb, settings.format, settings.filter }))
				{
					CONSOLE_LogError("CTexture", "Failed to import Texture asset!");
					CFileSystem::FindMod(data->outMod)->DeleteFile(path);
				}
			}
		}
	}
} static FTextureImportAction_instance;
