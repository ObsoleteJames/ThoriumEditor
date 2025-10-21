
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <Util/Core.h>
#include <Util/KeyValue.h>
#include "Build.h"
#include <iostream>
#include <string>
#include <filesystem>

FString GetGameNameFromProject(const FString& projectCfg)
{
	FKeyValue kv(projectCfg);
	if (!kv.IsOpen())
		return FString();

	auto* v = kv.GetValue("game", false);
	if (v)
		return *v;

	return FString();
}

#if _WIN32
#define THIS_PLATFORM PLATFORM_WIN64
#else
#define THIS_PLATFORM PLATFORM_LINUX
#endif

int main(int argc, char** argv)
{
	int targetFunction = argc > 1; // 1 = generate cmake files, 2 = gen & compile cmake files, 3 = Post build step (copy binaries and such)
	FString sourcePath;
	EPlatform targetPlatform = THIS_PLATFORM;
	EConfig targetConfig = CONFIG_DEBUG;
	ECompiler targetCompiler = COMPILER_DEFAULT;

	// by default the latest version
	FString engineVersion = "1.0";
	FString additionalSources;

	FString arg;

	if (argc == 1)
	{
		std::cout << "Thorium Engine - Build Tool 1.0\n";
		std::cout << "- commands:\n";

		std::cout << "\tgenerate\t-\tgenerate [BUILD.CFG path] options... // generates CMake projects\n";
		std::cout << "\tbuild\t\t-\tbuild [BUILD.CFG path] options... // generates and compiles CMake projects\n";

	_get_input:
		std::string cmd;
		std::getline(std::cin, cmd);

		TArray<FString> args = FString(cmd.c_str()).Split(" \t");
		
		if (args.Size() > 0)
		{
			if (args[0] == "tes")
			{
				targetFunction = 1;

				arg = "../../ThoriumEngine/Build.cfg";
			}

			if (args[0] == "generate")
			{
				targetFunction = 1;
				
				if (args.Size() > 1)
					arg = args[1];
			}
			if (args[0] == "build")
			{
				targetFunction = 2;

				if (args.Size() > 1)
					arg = args[1];
			}

#if _WIN32
#else
			if (args[0] == "cd")
			{
				char dir[128];
				std::cout << getcwd(dir, sizeof(dir)) << "\n";
				goto _get_input;
			}
#endif
		}
	}

	if (argc > 1)
		arg = argv[1];
		//sourcePath = argv[1];

	ETarget target = TARGET_NONE;
	if (auto i = arg.FindLastOf("\\/"); i != -1)
	{
		FString name = arg;
		name.Erase(name.begin(), name.begin() + i + 1);

		if (name == "project.cfg")
			target = TARGET_PROJECT;
		else if (name == "Build.cfg")
			target = TARGET_BUILD_CFG;
	}

	for (int i = 2; i < argc; i++)
	{
		bool bLastArg = i == argc - 1;
		FString arg = argv[i];
		if (arg == "-gen")
			targetFunction = 1;
		else if (arg == "-build")
			targetFunction = 2;
		// else if (arg == "-pb")
		// 	targetFunction = 3;
		//else if (arg == "-gensln")
		//	targetFunction = 3;

		else if (arg == "-engine" && !bLastArg)
			engineVersion = argv[i + 1];

		else if (arg == "-AS" && !bLastArg)
			additionalSources = std::filesystem::absolute(argv[i + 1]).generic_string().c_str();

		else if (arg == "-debug")
			targetConfig = CONFIG_DEBUG;
		else if (arg == "-development")
			targetConfig = CONFIG_DEVELOPMENT;
		else if (arg == "-release")
			targetConfig = CONFIG_RELEASE;

		else if (arg == "-x64")
			targetPlatform = PLATFORM_WIN64;
		else if (arg == "-linux")
			targetPlatform = PLATFORM_LINUX;
		else if (arg == "-mac")
			targetPlatform = PLATFORM_MAC;

		else if (arg == "-msvc")
			targetCompiler = COMPILER_MSVC;
		else if (arg == "-gcc")
			targetCompiler = COMPILER_GCC;
	}
	
	if (targetFunction == 1 || targetFunction == 2)
	{
		if (target == TARGET_PROJECT)
		{
			if (!GenerateBuildFromProject(arg))
			{
				std::cout << "Failed to generate build.cfg for project '" << arg.c_str() << "'\n";
				return 1;
			}

			FString gameName = GetGameNameFromProject(arg);
			sourcePath = arg;
			if (auto i = sourcePath.FindLastOf("\\/"); i != -1)
				sourcePath.Erase(sourcePath.begin() + i, sourcePath.end());
			if (auto i = sourcePath.FindLastOf("\\/"); i != -1)
				sourcePath.Erase(sourcePath.begin() + i, sourcePath.end());

			sourcePath += "\\.project\\" + gameName + "\\Build.cfg";
		}
		else
			sourcePath = arg;

		if (!sourcePath.IsEmpty())
		{
			FCompileConfig cfg;
			cfg.compiler = targetCompiler;
			cfg.config = targetConfig;
			cfg.platform = targetPlatform;
			cfg.engineVersion = engineVersion;
			cfg.path = std::filesystem::absolute(sourcePath.c_str()).generic_string().c_str();
			cfg.additionalSources = additionalSources;

			int i = GenerateCMakeProject(cfg);
			if (i != 0)
				std::cin.get();
			return i;
		}
	}
	if (targetFunction == 2)
	{
		
	}
	// if (targetFunction == 3)
	// {

	// }

	return 1;
}
