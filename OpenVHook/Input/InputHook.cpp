#include "InputHook.h"
#include "..\Utility\Log.h"
#include "..\Scripting\ScriptManager.h"

using namespace Utility;

WNDPROC	oWndProc;

KeyState keyboardState[0xFF];

LRESULT APIENTRY InputHook::WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {

	if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP) {

		WndKeyEvent((DWORD)wParam, lParam & 0xFFFF, lParam >> 16 & 0xFF, lParam >> 24 & 1,
			uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP, lParam >> 30 & 1, uMsg == WM_SYSKEYUP || uMsg == WM_KEYUP);
	}

	return CallWindowProc( oWndProc, hwnd, uMsg, wParam, lParam );
}

void InputHook::WndKeyEvent(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow)
{
	if (key < 0xFF)
	{
		keyboardState[key].lastUpTime = GetTickCount();
		keyboardState[key].isWithAlt = isWithAlt;
		keyboardState[key].wasDownBefore = wasDownBefore;
		keyboardState[key].isUpNow = isUpNow;
	}

	ScriptManager::HandleKeyEvent(key, repeats, scanCode, isExtended, isWithAlt, wasDownBefore, isUpNow);
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

