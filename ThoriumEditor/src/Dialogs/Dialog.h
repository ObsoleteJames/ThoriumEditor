#pragma once

#include "Math/Vectors.h"

class CWindow;
class ImGuiContext;
class ISwapChain;

#define DIALOG_WND_FLAGS (1 | 1 << 1 | 1 << 2)

enum EDialogFlags_
{
	EDialogFlags_NoResize = 1,
	EDialogFlags_IgnoreClose = 1 << 1,
};
typedef uint8 EDialogFlags;

class CDialogWnd
{
public:
	CDialogWnd() = default;
	CDialogWnd(const FString& title, EDialogFlags flags = 0);

	int Exec();

protected:
	void Finish(int returnCode = 0);

	virtual void Render();

public:
	FVector2 windowSize = { 400, 258 };
	FString title;

	EDialogFlags flags;

private:
	CWindow* window;
	ImGuiContext* context;

	ISwapChain* swapChain;

	bool bFinished = 0;
	int returnCode = 0;
};
