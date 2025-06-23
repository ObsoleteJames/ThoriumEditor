#pragma once

#include "Dialog.h"

struct FStructTree;

class CSelectClassDialog : public CDialogWnd
{
public:
	CSelectClassDialog(const FString& title, FClass* filter = nullptr);

	inline FClass* Selected() const { return selected; }

protected:
	void Render() override;

	bool IsSelectionValid();

private:
	FClass* filter;
	FClass* selected = nullptr;

	FStructTree* tree;
};

class CSelectStructDialog : public CDialogWnd
{
public:
	CSelectStructDialog(const FString& title, FStruct* filter = nullptr);

protected:
	void Render() override;

private:
	FStruct* filter;
	FStruct* selected = nullptr;

	FStructTree* tree;
};

class CSelectEnumDialog : public CDialogWnd
{
public:
	CSelectEnumDialog(const FString& title);

protected:
	void Render() override;

};
