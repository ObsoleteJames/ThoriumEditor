#pragma once

#include <QDialog>
#include "FramelessDialog.h"
#include <Util/Core.h>

struct FFile;
struct FAssetClass;
class CContentBrowserWidget;
class QLineEdit;

class CSaveFileDialog : public CFramelessDialog
{
	Q_OBJECT

public:
	CSaveFileDialog(QWidget* parent = nullptr);

	inline FString Path() const { return path; }

private:
	void Save();

private:
	FString path;
	CContentBrowserWidget* assetBrowser;
	QLineEdit* nameEdit;

};

class COpenFileDialog : public CFramelessDialog
{
	Q_OBJECT

public:
	//COpenFileDialog(const FString& filter, QWidget* parent = nullptr);
	COpenFileDialog(FAssetClass* filterType, QWidget* parent = nullptr);

	inline FFile* File() const { return file; }
	
private:
	FFile* file;
	CContentBrowserWidget* assetBrowser;

};
