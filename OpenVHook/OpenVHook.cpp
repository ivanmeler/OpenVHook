#include "Input\InputHook.h"
#include "ASI Loader\ASILoader.h"
#include "Scripting\ScriptEngine.h"
#include "Scripting\ScriptManager.h"
#include "Utility\Console.h"
#include "Utility\General.h"
#include "Utility\Pattern.h"

using namespace Utility;

DWORD WINAPI Run() {

#ifdef _DEBUG
	GetConsole()->Allocate();
#endif

	GetLog()->Print( "Initializing..." );

	if ( !InputHook::Initialize() ) {

		GetLog()->Error( "Failed to initialize InputHook" );
		return 0;
	}

	if ( !ScriptEngine::Initialize() ) {

		GetLog()->Error( "Failed to initialize ScriptEngine" );
		return 0;
	}

	ScriptEngine::CreateThread( &g_ScriptManagerThread );

	ASILoader::Initialize();

	GetLog()->Print( "Initialization finished" );

	return 1;
}

void Cleanup() {

	GetLog()->Print( "Cleanup" );

	// Maybe kill threads n shit

	InputHook::Remove();

	if ( GetConsole()->IsAllocated() ) {
		GetConsole()->DeAllocate();
	}
}

BOOL APIENTRY DllMain( HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved ) {

	switch ( dwReason ) {
		case DLL_PROCESS_ATTACH: {

			SetOurModuleHanlde( hModule );
			CreateThread( NULL, NULL, (LPTHREAD_START_ROUTINE)Run, NULL, NULL, NULL );
			break;
		}
		case DLL_PROCESS_DETACH: {

			Cleanup();
			break;
		}
	}

	return TRUE;
}
