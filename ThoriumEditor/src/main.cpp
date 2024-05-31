
#include <Util/KeyValue.h>
#include "EditorEngine.h"
#include "Misc/CommandLine.h"
#include "Console.h"

#ifdef _WIN32
#include <Windows.h>
#endif

int ParseArgs(FString& targetProj)
{
	int r = 0;
	const auto& args = FCommandLine::GetArgs();
	if (args.Size() > 0)
	{
		if (SizeType i = args[0].Find(".thproj"); i != -1)
		{
			FKeyValue kv(args[0], EKeyValueType::KV_STANDARD_ASCII);
			if (auto* v = kv.GetValue("engine_version", false); v != nullptr)
			{
				if (v->Value != ENGINE_VERSION)
				{
					FString enginePath = CEngine::OSGetEnginePath(v->Value);
					THORIUM_ASSERT(!enginePath.IsEmpty(), "The engine version this project requires is not install on this computer.");

					FString args = "";

#ifdef _WIN32
					STARTUPINFO si{};
					PROCESS_INFORMATION pi{};

					si.cb = sizeof(si);
					CreateProcessA((enginePath + "/bin/Thorium Editor.exe").c_str(), (char*)args.c_str(), nullptr, nullptr, false, CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &si, &pi);
#endif
					return 1;
				}
				targetProj = args[0];
				targetProj.Erase(targetProj.begin() + targetProj.FindLastOf("/\\"), targetProj.end());
			}
		}
	}

	if (targetProj.IsEmpty())
	{
		targetProj = "../.project";
	}
	return r;
}

#ifdef _WIN32
#include "iomanip"
#include <sstream>
#include "minidumpapiset.h"

LONG WINAPI Win32ExceptionHandler(_EXCEPTION_POINTERS* exceptionInfo)
{
	typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

	HMODULE mhlib = LoadLibrary("dbghelp.dll");
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

	HANDLE hFile = CreateFile(("crash " + timeTxt + ".dmp").c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

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
	FCommandLine::Parse(lpCmdLine);
#else
	FCommandLine::Parse(argv, argc);
#endif

	int exitCode = 0;

	FString project;
	if (ParseArgs(project) != 0)
		return 0;

	gEngine = new CEditorEngine();
	gEngine->Init();

	try {
		gIsMainGaurded = true;
		gEngine->LoadProject(project + "/..");

		exitCode = gEngine->Run();
	}
	catch (std::exception& e) {
		CONSOLE_LogError("CORE", e.what());
		gEngine->SaveConsoleLog();
		return 1;
	}
	catch (...) {
		CONSOLE_LogError("CORE", "Unexpected exception has occured!");
		gEngine->SaveConsoleLog();
		return 1;
	}

	return exitCode;
}
