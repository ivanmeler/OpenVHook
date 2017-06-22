#include "ASILoader.h"
#include "..\Utility\Log.h"
#include "..\Utility\General.h"
#include "..\Utility\PEImage.h"

using namespace Utility;

void ASILoader::Initialize() {

	LOG_PRINT( "Loading plugins" );

	const std::string currentFolder = GetRunningExecutableFolder();
	const std::string asiFolder = currentFolder + "\\asi";

	const std::string asiSearchQuery = asiFolder + "\\*.asi";

	WIN32_FIND_DATAA fileData;
	HANDLE fileHandle = FindFirstFileA( asiSearchQuery.c_str(), &fileData );
	if ( fileHandle != INVALID_HANDLE_VALUE ) {

		do {

			const std::string pluginPath = asiFolder + "\\" + fileData.cFileName;

			LOG_PRINT( "Loading \"%s\"", pluginPath.c_str() );

			PEImage pluginImage;
			if ( !pluginImage.Load( pluginPath ) ) {

				LOG_ERROR( "\tFailed to load image" );
				continue;
			}

			// Image compatible (now), load it
			HMODULE module = LoadLibraryA( pluginPath.c_str() );
			if ( module ) {
				LOG_PRINT( "\tLoaded \"%s\" => 0x%p", fileData.cFileName, module );
			} else {
				DWORD errorMessageID = ::GetLastError();
				if (errorMessageID == 0)
					LOG_ERROR( "\tFailed to load" );

				LPSTR messageBuffer = nullptr;
				size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

				std::string message(messageBuffer, size);

				//Free the buffer.
				LocalFree(messageBuffer);
				LOG_ERROR( "\tFailed to load: %s", message.c_str() );
			}

		} while ( FindNextFileA( fileHandle, &fileData ) );

		FindClose( fileHandle );
	}

	LOG_PRINT( "Finished loading plugins" );
	LOG_PRINT("SEAL__");
}
