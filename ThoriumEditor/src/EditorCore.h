#pragma once

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

#if _WIN32
	#ifdef THORIUMEDITOR_DLL
		#define SDK_API __declspec(dllexport)
	#else
		#define SDK_API __declspec(dllimport)
	#endif
#elif __GNUC__
	#ifdef THORIUMEDITOR_DLL
		#define SDK_API __atribute__((visibility("default")))
	#else
		#define SDK_API
	#endif
#else
	#define SDK_API
#endif
