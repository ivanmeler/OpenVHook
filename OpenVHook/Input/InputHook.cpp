#include "InputHook.h"
#include "..\Utility\Log.h"
#include "..\Scripting\ScriptManager.h"

using namespace Utility;

WNDPROC	oWndProc;

LRESULT APIENTRY InputHook::WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {

	ScriptManager::WndProc( hwnd, uMsg, wParam, lParam );

	return CallWindowProc( oWndProc, hwnd, uMsg, wParam, lParam );
}

bool InputHook::Initialize() {

	HWND windowHandle = NULL;
	while ( windowHandle == NULL ) {

		windowHandle = FindWindow( "grcWindow", NULL );
		Sleep( 100 );
	}

	oWndProc = (WNDPROC)SetWindowLongPtr( windowHandle, GWLP_WNDPROC, (LONG_PTR)WndProc );
	if ( oWndProc == NULL ) {

		LOG_ERROR( "Failed to attach input hook" );
		return false;
	} else {

		LOG_PRINT( "Input hook attached: WndProc 0x%p", (DWORD_PTR)oWndProc );
		return true;
	}
}

void InputHook::Remove() {

	HWND windowHandle = FindWindow( "grcWindow", NULL );
	SetWindowLongPtr( windowHandle, GWLP_WNDPROC, (LONG_PTR)oWndProc );
	LOG_DEBUG( "Removed input hook" );
}

