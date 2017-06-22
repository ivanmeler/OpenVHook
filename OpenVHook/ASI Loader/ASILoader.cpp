#include "ASILoader.h"
#include "..\Utility\Log.h"
#include "..\Utility\General.h"
#include "..\Utility\PEImage.h"

using namespace Utility;

static std::vector<std::string> plugins;

static bool bLoaded = false;

void ASILoader::Initialize() {

	LOG_PRINT("Loading *.asi plugins");

	const std::string currentFolder = GetRunningExecutableFolder();
	const std::string asiFolder = currentFolder + "\\asi";

	auto findPlugins = [](std::string path) {

		const std::string asiSearchQuery = path + "\\*.asi";

		WIN32_FIND_DATAA fileData;
		HANDLE fileHandle = FindFirstFileA(asiSearchQuery.c_str(), &fileData);
		if (fileHandle != INVALID_HANDLE_VALUE) {

			do {
				const std::string pluginPath = path + "\\" + fileData.cFileName;

				LOG_PRINT("Loading \"%s\"", pluginPath.c_str());

				PEImage pluginImage;
				if (!pluginImage.Load(pluginPath)) {

					LOG_ERROR("\tFailed to load image");
					continue;
				}

				plugins.push_back(pluginPath);

			} while (FindNextFileA(fileHandle, &fileData));

			FindClose(fileHandle);
		}
	};

	findPlugins(currentFolder);
	findPlugins(asiFolder);

	LOG_PRINT("Finished loading *.asi plugins");
}

void ASILoader::LoadPlugins()
{
	if (Loaded()) UnloadPlugins();

	for (auto& pluginPath : plugins)
	{
		auto module = LoadLibraryA( pluginPath.c_str() );

		if ( module ) {
			LOG_PRINT("\tLoaded \"%s\" => 0x%p", GetModuleName(module).c_str(), module);
		}
		else {
			DWORD errorMessageID = ::GetLastError();
			if (errorMessageID == 0)
				LOG_ERROR("\tFailed to load");

			LPSTR messageBuffer = nullptr;
			size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

			std::string message(messageBuffer, size);

			//Free the buffer.
			LocalFree(messageBuffer);
			LOG_ERROR("\tFailed to load \"%s\": %s", GetFilename(pluginPath).c_str(), message.c_str());
		}
	}

	bLoaded = true;
}

void ASILoader::UnloadPlugins()
{
	for (auto& plugin : plugins)
	{
		HINSTANCE hModule;

		if ((hModule = GetModuleHandleA(plugin.c_str())))
		{
			FreeLibrary( hModule );
		}
	}

	bLoaded = false;
}

bool ASILoader::Loaded()
{
	return bLoaded;
}
