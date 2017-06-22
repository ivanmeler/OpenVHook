#ifndef __INPUT_HOOK_H__
#define __INPUT_HOOK_H__

#include "..\OpenVHook.h"

typedef struct KeyState
{
	int lastUpTime;
	BOOL isWithAlt;
	BOOL wasDownBefore;
	BOOL isUpNow;
} KeyState_t;

extern KeyState				keyboardState[];

namespace InputHook {

	bool					Initialize();

	void					Remove();

	static LRESULT APIENTRY WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

	void WndKeyEvent(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow);
}

#endif // __INPUT_HOOK_H__