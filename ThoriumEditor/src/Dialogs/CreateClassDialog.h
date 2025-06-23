#pragma once

#include "Dialog.h"

class CCreateClassDialog : public CDialogWnd
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
	CCreateClassDialog();

protected:
	void Render() override;

private:
	FClass* base;

};
