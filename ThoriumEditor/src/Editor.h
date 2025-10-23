#pragma once

//#include <QThread>
#include <Util/Core.h>
class CEngineThread;

#ifdef _WIN32
	#ifdef THORIUMEDITORQT_DLL
		#define EDITOR_API __declspec(dllexport)
	#else
		#define EDITOR_API __declspec(dllimport)
	#endif
#else
	#define EDITOR_API
#endif

extern EDITOR_API CEngineThread* gEngineThread;

EDITOR_API bool StartEngineThread(const FString& project = FString());
EDITOR_API void StopEngineThread();
EDITOR_API void DestroyEngineThread();
	
enum EItemTypes
{
	EItemTypes_Folder = 1000,
	EItemTypes_ModFolder,
	EItemTypes_GenericFile,
	EItemTypes_AssetFile,
	EItemTypes_Entity,
	EItemTypes_EntityComponent,
	EItemTypes_SceneComponent
};
