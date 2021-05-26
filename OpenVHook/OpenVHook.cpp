#include "Input\InputHook.h"
#include "Scripting\ScriptEngine.h"
#include "Scripting\ScriptManager.h"
#include "Utility\Console.h"
#include "Utility\General.h"
#include "Utility\Pattern.h"
#include "Utility\Thread.h"
#include "DirectXHook/DirectXHook.h"

using namespace Utility;

static Thread reloadPluginThread = Thread([](ThreadState*) {

	if (GetTickCount64() - keyboardState[VK_CONTROL].lastUpTime < 5000 && !keyboardState[VK_CONTROL].isUpNow &&
		GetTickCount64() - keyboardState['R'].lastUpTime < 100 && keyboardState['R'].isUpNow)
	{
		keyboardState[0x52].lastUpTime = 0;

		if (g_ScriptManagerThread.Count()) {
			LOG_PRINT("Unloading .asi plugins...");
			g_ScriptManagerThread.FreeScripts();
			MessageBeep(0);
		}
		else {
			LOG_PRINT("Reloading .asi plugins...");
			g_ScriptManagerThread.LoadScripts();
			for (int i = 0; i < 3; i++) {
				MessageBeep(0);
				Sleep(200);
			}
		}
	}
});

static Thread initThread = Thread([](ThreadState* ts) {
	ts->shouldExit = 1;

#ifdef _DEBUG
	GetConsole()->Allocate();
#endif

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

	LOG_PRINT( "Initialization finished" );

	reloadPluginThread.Run();
});

void Cleanup() {

	LOG_PRINT( "Cleanup" );

	initThread.Exit();

	reloadPluginThread.Exit();

	InputHook::Remove();

	g_D3DHook.ReleaseDevices(true);

	if ( GetConsole()->IsAllocated() ) {
		GetConsole()->DeAllocate();
	}
}

// OpenVHook is loaded into game process as a dependency,
// without this function, refcount of OpenVHook will dec to 0 after the last asi plugins being unload.
// then OpenVHook itself will be unload immediately, which breaks hot-reload
static void EnsureSelfReference(HINSTANCE hModule) {
	auto fullpath = GetModuleFullName(hModule);
	// this increase ref count of OpenVHook
	static auto g_hmodule_self = LoadLibrary(fullpath.c_str());
	LOG_PRINT("Kept OpenVHook reference: %p", g_hmodule_self);
}

static void CancelSelfReference(HINSTANCE hModule) {
	// dec ref count
	FreeLibrary(hModule);
}

BOOL APIENTRY DllMain( HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved ) {

	switch ( dwReason ) {
		case DLL_PROCESS_ATTACH: {
			EnsureSelfReference(hModule);
			SetOurModuleHandle( hModule );
			initThread.Run();
			break;
		}
		case DLL_PROCESS_DETACH: {
			CancelSelfReference(hModule);
			Cleanup();
			break;
		}
	}

	return TRUE;
}
