
#include "Editor.h"
#include "Engine.h"
#include "EditorWindow.h"
#include "EngineThread.h"
#include "Misc/CommandLine.h"
#include "System.h"
#include "ProjectManagerWindow.h"
#include <Util/KeyValue.h>

#include <QApplication>

#ifdef _WIN32
#include "windows.h"
#include "iomanip"
#include <sstream>
#include "minidumpapiset.h"

LONG WINAPI Win32ExceptionHandler(_EXCEPTION_POINTERS* exceptionInfo)
{
	typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

	HMODULE mhlib = LoadLibraryA("dbghelp.dll");
	MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)GetProcAddress(mhlib, "MiniDumpWriteDump");

	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);

	std::ostringstream oss;
	oss << std::put_time(&tm, "%d-%m-%y %H-%M-%S");
	std::string timeTxt = oss.str();

	_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
	ExInfo.ThreadId = ::GetCurrentThreadId();
	ExInfo.ExceptionPointers = exceptionInfo;
	ExInfo.ClientPointers = FALSE;

	HANDLE hFile = CreateFileA(("crash " + timeTxt + ".dmp").c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL);
	CloseHandle(hFile);
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
#else
int main(int argc, char** argv)
#endif
{
#ifdef _WIN32
#ifdef IS_DEV
	if (!IsDebuggerPresent())
	{
		//MessageBoxA(nullptr, "", "No debugger attached", MB_OK);

		SetUnhandledExceptionFilter(Win32ExceptionHandler);
	}
#endif
#endif

#ifdef _WIN32
	FCommandLine::Parse(lpCmdLine, false);
	int argc = 0;
	char** argv = nullptr;
#else
	FCommandLine::Parse(argv, argc);
#endif

	gIsEditor = true;

	QApplication app(argc, argv);

	int openProjectManager = true;

	CEditorVar::Load();
	CEditorWindow::LoadStyleSheet();

	FKeyValue kv(SSystem::GetDataPath() + "/ThoriumEngine/EditorConfig/Editor.cfg");
	if (kv.IsOpen())
		openProjectManager = kv.GetValue("show_projectbrowser_startup")->AsBool(true);

	if (!openProjectManager)
	{
		StartEngineThread();
		CEditorWindow* window = CToolsWindow::Create<CEditorWindow>();
	}
	else
		CToolsWindow::Create<CProjectManagerWnd>();

	int r = app.exec();

	//gEngine->Exit();

	//gEngineThread->quit();
	//gEngineThread->wait();
	return r;
}
