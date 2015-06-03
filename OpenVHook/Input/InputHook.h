#ifndef __INPUT_HOOK_H__
#define __INPUT_HOOK_H__

#include "..\OpenVHook.h"

namespace InputHook {

	bool					Initialize();

	void					Remove();

	static LRESULT APIENTRY WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
}

#endif // __INPUT_HOOK_H__