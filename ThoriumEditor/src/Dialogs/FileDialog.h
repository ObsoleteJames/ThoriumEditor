#pragma once

#include "Dialog.h"

class CAssetBrowserWidget;

class CSaveFileDialog : public CDialogWnd
{
public:
	CSaveFileDialog(FClass* type, const FString& dir = FString());

	inline const FString& Path() { return outPath; }
	inline const FString& Mod() { return mod; }

protected:
	void Render() override;

private:
	CAssetBrowserWidget* browser;
	FString mod;
	FString outPath;
	FString fileName;

};
