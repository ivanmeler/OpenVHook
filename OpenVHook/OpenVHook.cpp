#include "Input\InputHook.h"
#include "ASI Loader\ASILoader.h"
#include "Scripting\ScriptEngine.h"
#include "Scripting\ScriptManager.h"
#include "Utility\Console.h"
#include "Utility\General.h"
#include "Utility\Pattern.h"

using namespace Utility;

DWORD WINAPI Run() {

	GetConsole()->Allocate();

	LOG_PRINT( "Initializing..." );

	if ( !InputHook::Initialize() ) {

		LOG_ERROR( "Failed to initialize InputHook" );
		return 0;
	}

	if ( !ScriptEngine::Initialize() ) {

		LOG_ERROR( "Failed to initialize ScriptEngine" );
		return 0;
	}

	ScriptEngine::CreateThread( &g_ScriptManagerThread );

	ASILoader::Initialize();

	LOG_PRINT( "Initialization finished" );

	return 1;
}

void Cleanup() {

	LOG_PRINT( "Cleanup" );


	InputHook::Remove();

	/*if ( GetConsole()->IsAllocated() ) {
		GetConsole()->DeAllocate();
	}*/
}

BOOL APIENTRY DllMain( HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved ) {

	switch ( dwReason ) {
		case DLL_PROCESS_ATTACH: {

			SetOurModuleHanlde( hModule );
			CloseHandle( CreateThread( NULL, NULL, (LPTHREAD_START_ROUTINE)Run, NULL, NULL, NULL ) );
			break;
		}
		case DLL_PROCESS_DETACH: {

			Cleanup();
			break;
		}
	}

	return TRUE;
}
