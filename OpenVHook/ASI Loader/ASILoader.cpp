#include "ASILoader.h"
#include "..\Utility\Log.h"
#include "..\Utility\General.h"
#include "..\Utility\PEImage.h"

using namespace Utility;

void ASILoader::Initialize() {

	LOG_PRINT( "Loading *.asi plugins" );

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

			// Image not compatible, needs patching
			if ( !pluginImage.IsOpenVHookCompatible() ) {

				LOG_PRINT( "\tDetected non compatible image. Patching compatibility" );

				if ( pluginImage.PatchCompatibility() ) {

					LOG_PRINT( "\tSuccessfully patched" );

				} else {

					LOG_ERROR( "\tFailed to patch compatibility" );
					continue;
				}
			}

			// Image compatible (now), load it
			HMODULE module = LoadLibraryA( pluginPath.c_str() );
			if ( module ) {
				LOG_PRINT( "\tLoaded \"%s\" => 0x%p", fileData.cFileName, module );
			} else {
				LOG_ERROR( "\tFailed to load" );
			}

		} while ( FindNextFileA( fileHandle, &fileData ) );

		FindClose( fileHandle );
	}

	LOG_PRINT( "Finished loading *.asi plugins" );
}
