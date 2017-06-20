#include "ScriptManager.h"
#include "..\Utility\Log.h"
#include "..\Utility\General.h"

using namespace Utility;

#pragma comment(lib, "winmm.lib")
#define DLL_EXPORT __declspec( dllexport )

ScriptManagerThread g_ScriptManagerThread;

static HANDLE		mainFiber;
static Script *		currentScript;

void Script::Tick() {

	if ( mainFiber == nullptr ) {
		mainFiber = ConvertThreadToFiber( nullptr );
	}

	if ( timeGetTime() < wakteAt ) {
		return;
	}

	if ( scriptFiber ) {

		currentScript = this;
		SwitchToFiber( scriptFiber );
		currentScript = nullptr;
	} else {

		scriptFiber = CreateFiber( NULL, []( LPVOID handler ) {

			__try {

				reinterpret_cast<Script*>( handler )->Run();

			} __except ( EXCEPTION_EXECUTE_HANDLER ) {	

				LOG_ERROR( "Error in script->Run" );
			}
		}, this );
	}
}

void Script::Run() {

	callbackFunction();
}

void Script::Yield( uint32_t time ) {

	wakteAt = timeGetTime() + time;
	SwitchToFiber( mainFiber );
}

void ScriptManagerThread::DoRun() {

	scriptMap thisIterScripts( m_scripts );

	for ( auto & pair : thisIterScripts ) {
		pair.second->Tick();
	}
}

eThreadState ScriptManagerThread::Reset( uint32_t scriptHash, void * pArgs, uint32_t argCount ) {

	// Collect all scripts
	scriptMap tempScripts;

	for ( auto && pair : m_scripts ) {
		tempScripts[pair.first] = pair.second;
	}

	// Clear the scripts
	m_scripts.clear();

	// Start all scripts
	for ( auto && pair : tempScripts ) {
		AddScript( pair.first, pair.second->GetCallbackFunction() );
	}

	return ScriptThread::Reset( scriptHash, pArgs, argCount );
}

void ScriptManagerThread::AddScript( HMODULE module, void( *fn )( ) ) {

	const std::string moduleName = GetModuleNameWithoutExtension( module );

	LOG_PRINT( "Registering script '%s' (0x%p)", moduleName.c_str(), fn );

	if ( m_scripts.find( module ) != m_scripts.end() ) {

		LOG_ERROR( "Script '%s' is already registered", moduleName.c_str() );
		return;
	}

	m_scripts[module] = std::make_shared<Script>( fn );
}

void ScriptManagerThread::RemoveScript( void( *fn )( ) ) {

	for ( auto it = m_scripts.begin(); it != m_scripts.end(); it++ ) {

		auto pair = *it;
		if ( pair.second->GetCallbackFunction() == fn ) {

			RemoveScript( pair.first );
		}
	}
}

void ScriptManagerThread::RemoveScript( HMODULE module ) {

	auto pair = m_scripts.find( module );
	if ( pair == m_scripts.end() ) {

		LOG_ERROR( "Could not find script for module 0x%p", module );
		return;
	}

	LOG_PRINT( "Unregistered script '%s'", GetModuleNameWithoutExtension( module ).c_str() );
	m_scripts.erase( pair );
}

void DLL_EXPORT scriptWait( unsigned long waitTime ) {

	currentScript->Yield( waitTime );
}

void DLL_EXPORT scriptRegister( HMODULE module, void( *function )( ) ) {

	g_ScriptManagerThread.AddScript( module, function );
}

void DLL_EXPORT scriptUnregister( void( *function )( ) ) {

	g_ScriptManagerThread.RemoveScript( function );
}

void DLL_EXPORT scriptUnregister( HMODULE module ) {
	
	g_ScriptManagerThread.RemoveScript( module );
}

enum eGameVersion : int {
	G_VER_1_0_335_2_STEAM, // 00
	G_VER_1_0_335_2_NOSTEAM, // 01

	G_VER_1_0_350_1_STEAM, // 02
	G_VER_1_0_350_2_NOSTEAM, // 03

	G_VER_1_0_372_2_STEAM, // 04
	G_VER_1_0_372_2_NOSTEAM, // 05

	G_VER_1_0_393_2_STEAM, // 06
	G_VER_1_0_393_2_NOSTEAM, // 07

	G_VER_1_0_393_4_STEAM, // 08
	G_VER_1_0_393_4_NOSTEAM, // 09

	G_VER_1_0_463_1_STEAM, // 10
	G_VER_1_0_463_1_NOSTEAM, // 11

	G_VER_1_0_505_2_STEAM, // 12
	G_VER_1_0_505_2_NOSTEAM, // 13

	G_VER_1_0_573_1_STEAM, // 14
	G_VER_1_0_573_1_NOSTEAM, // 15

	G_VER_1_0_617_1_STEAM, // 16
	G_VER_1_0_617_1_NOSTEAM, // 17

	G_VER_1_0_678_1_STEAM, // 18
	G_VER_1_0_678_1_NOSTEAM, // 19

	G_VER_1_0_757_2_STEAM, // 20
	G_VER_1_0_757_2_NOSTEAM, // 21

	G_VER_1_0_757_4_STEAM, // 22
	G_VER_1_0_757_4_NOSTEAM, // 23

	G_VER_1_0_791_2_STEAM, // 24
	G_VER_1_0_791_2_NOSTEAM, // 25

	G_VER_1_0_877_1_STEAM, // 26
	G_VER_1_0_877_1_NOSTEAM, // 27

	G_VER_1_0_944_2_STEAM, // 28
	G_VER_1_0_944_2_NOSTEAM, // 29

	G_VER_1_0_1011_1_STEAM, // 30
	G_VER_1_0_1011_1_NOSTEAM, // 31

	G_VER_1_0_1032_1_STEAM, // 32
	G_VER_1_0_1032_1_NOSTEAM, // 33

	G_VER_1_0_1103_2_STEAM, // 32
	G_VER_1_0_1103_2_NOSTEAM, // 33

};

eGameVersion DLL_EXPORT getGameVersion() {

	// TODO: Actually implement this??
	//LOG_WARNING( "Aint nothin here in 'getGameVersion' bruv" );
	return G_VER_1_0_1103_2_STEAM; //lmao
}

void DLL_EXPORT scriptRegisterAdditionalThread( HMODULE module, void( *function )( ) ) {

	// TODO: Implement this at some point, to lazy right now
	LOG_WARNING( "Aint nothin here in 'scriptRegisterAdditionalThread' bruv" );
}

static ScriptManagerContext g_context;
static uint64_t g_hash;

void DLL_EXPORT nativeInit( uint64_t hash ) {

	g_context.Reset();
	g_hash = hash;
}

void DLL_EXPORT nativePush64( uint64_t value ) {

	g_context.Push( value );
}

DLL_EXPORT uint64_t * nativeCall() {

	auto fn = ScriptEngine::GetNativeHandler( g_hash );

	if ( fn != 0 ) {

		__try {

			fn( &g_context );
		} __except ( EXCEPTION_EXECUTE_HANDLER ) {

			LOG_ERROR( "Error in nativeCall" );
		}
	}

	return reinterpret_cast<uint64_t*>( g_context.GetResultPointer() );
}

typedef void( *TKeyboardFn )( DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow );

static std::set<TKeyboardFn> g_keyboardFunctions;

void DLL_EXPORT keyboardHandlerRegister( TKeyboardFn function ) {

	g_keyboardFunctions.insert( function );
}

void DLL_EXPORT keyboardHandlerUnregister( TKeyboardFn function ) {

	g_keyboardFunctions.erase( function );
}

void ScriptManager::WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {

	if ( uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP ) {

		auto functions = g_keyboardFunctions;

		for ( auto & function : functions ) {
			function( (DWORD)wParam, lParam & 0xFFFF, ( lParam >> 16 ) & 0xFF, ( lParam >> 24 ) & 1, ( uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP ), ( lParam >> 30 ) & 1, ( uMsg == WM_SYSKEYUP || uMsg == WM_KEYUP ) );
		}
	}
}

// CitizenFX doesn't have globals normally - this is therefore a no-op
DLL_EXPORT uint64_t* getGlobalPtr(int)
{
	// let the user party on a dummy global (allocate some buffer size at the end, though)
	static uint64_t dummyGlobal[128];

	return dummyGlobal;
}

// dummy pool functions (need implementation)
int DLL_EXPORT worldGetAllVehicles(int* array, int arraySize)
{
	return 0;
}

int DLL_EXPORT worldGetAllPeds(int* array, int arraySize)
{
	return 0;
}

int DLL_EXPORT worldGetAllObjects(int* array, int arraySize)
{
	return 0;
}
