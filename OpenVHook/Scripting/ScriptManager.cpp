#include "ScriptManager.h"
#include "..\Utility\Log.h"

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
			reinterpret_cast<Script*>( handler )->Run();
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

	scriptVec thisIterScripts( m_scripts );

	for ( auto & script : thisIterScripts ) {
		script->Tick();
	}
}

eThreadState ScriptManagerThread::Reset( uint32_t scriptHash, void * pArgs, uint32_t argCount ) {

	// Collect all script functions
	std::vector<void( *)( )> scriptFunctions;

	for ( auto&& script : m_scripts ) {
		scriptFunctions.push_back( script->GetCallbackFunction() );
	}

	// Clear the script list
	m_scripts.clear();

	// Start all script functions
	for ( auto && fn : scriptFunctions ) {
		AddScript( fn );
	}

	return ScriptThread::Reset( scriptHash, pArgs, argCount );
}

void ScriptManagerThread::AddScript( void( *fn )( ) ) {

	m_scripts.push_back( std::make_shared<Script>( fn ) );
}

void ScriptManagerThread::RemoveScript( void( *fn )( ) ) {

	for ( auto it = m_scripts.begin(); it != m_scripts.end(); it++ ) {

		if ( ( *it )->GetCallbackFunction() == fn ) {

			m_scripts.erase( it );
			return;
		}
	}
}

void DLL_EXPORT scriptWait( unsigned long waitTime ) {

	currentScript->Yield( waitTime );
}

void DLL_EXPORT scriptRegister( HMODULE hMod, void( *function )( ) ) {

	GetLog()->Debug( "Registering script: 0x%p", hMod );
	g_ScriptManagerThread.AddScript( function );
}

void DLL_EXPORT scriptUnregister( void( *function )( ) ) {

	GetLog()->Debug( "Unregistering script fn: 0x%p", function );
	g_ScriptManagerThread.RemoveScript( function );
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

DLL_EXPORT uint64_t* nativeCall() {

	auto fn = ScriptEngine::GetNativeHandler( g_hash );

	if ( fn != 0 ) {

		__try {

			fn( &g_context );
		} __except ( EXCEPTION_EXECUTE_HANDLER ) {

			GetLog()->Error( "Error in nativeCall" );
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
