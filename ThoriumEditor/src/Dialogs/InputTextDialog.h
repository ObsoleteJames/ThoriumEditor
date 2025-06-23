#pragma once

#include "Dialog.h"

class CInputTextDialog : public CDialogWnd
{
public:
	CInputTextDialog(const FString& title, const FString& label = FString());

	inline const FString& GetText() const { return input; }

protected:
	void Render() override;

private:
	FString label;
	FString input;

};
