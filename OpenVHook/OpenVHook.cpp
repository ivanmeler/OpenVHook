#include "Input\InputHook.h"
#include "ASI Loader\ASILoader.h"
#include "Scripting\ScriptEngine.h"
#include "Scripting\ScriptManager.h"
#include "Utility\Console.h"
#include "Utility\General.h"
#include "Utility\Pattern.h"
#include "Utility\Thread.h"

using namespace Utility;

static Thread mainThread = Thread([](ThreadState*) {

	if (GetTickCount() - keyboardState[VK_CONTROL].lastUpTime < 5000 && !keyboardState[VK_CONTROL].isUpNow &&
		GetTickCount() - keyboardState[0x52].lastUpTime < 100 && keyboardState[0x52].isUpNow)
	{
		keyboardState[0x52].lastUpTime = 0;

		if (g_ScriptManagerThread.LoadScripts())
		{
			int count = 3;
			while (count > 0)
			{
				MessageBeep(0);
				Sleep(200);
				--count;
			}
		}

		else
		{
			LOG_PRINT("Unloading .asi plugins...");

			g_ScriptManagerThread.FreeScripts();

			MessageBeep(0);
		}
	}
});

static Thread initThread = Thread([](ThreadState* ts) {
	ts->shouldExit = 1;

//#ifdef _DEBUG
	GetConsole()->Allocate();
//#endif

	LOG_PRINT( "Initializing..." );

	if ( !InputHook::Initialize() ) {

		LOG_ERROR( "Failed to initialize InputHook" );
		return;
	}

	if ( !ScriptEngine::Initialize() ) {

		LOG_ERROR( "Failed to initialize ScriptEngine" );
		return;
	}

	ScriptEngine::CreateThread( &g_ScriptManagerThread );

	ASILoader::Initialize();

	//ASILoader::LoadPlugins();

	LOG_PRINT( "Initialization finished" );

	mainThread.Run();
});

void Cleanup() {

	LOG_PRINT( "Cleanup" );

	initThread.Exit();

	mainThread.Exit();

	InputHook::Remove();

	if ( GetConsole()->IsAllocated() ) {
		GetConsole()->DeAllocate();
	}
}

BOOL APIENTRY DllMain( HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved ) {

	switch ( dwReason ) {
		case DLL_PROCESS_ATTACH: {

			SetOurModuleHandle( hModule );
			initThread.Run();
			break;
		}
		case DLL_PROCESS_DETACH: {

			Cleanup();
			break;
		}
	}

	return TRUE;
}
