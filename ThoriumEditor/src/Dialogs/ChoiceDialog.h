#pragma once

#include "Dialog.h"

class CChoiceDialog : public CDialogWnd
{
public:
	enum EOption
	{
		OPTION_OK,
		OPTION_YES_NO,
		OPTION_OK_CANCEL,
		OPTION_YES_NO_CANCEL,
	};

public:
	CChoiceDialog(const FString& title, const FString& msg, EOption option);

protected:
	void Render() override;

private:
	FString msg;
	EOption option;

};
