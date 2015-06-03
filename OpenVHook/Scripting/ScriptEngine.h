#ifndef __SCRIPT_ENGINE_H__
#define __SCRIPT_ENGINE_H__

#include "..\OpenVHook.h"

#include "ScriptThread.h"
#include "pgCollection.h"
#include "NativeInvoker.h"

enum eGameState {
	GameStatePlaying,
	GameStateIntro,
	GameStateLicenseShit = 3,
	GameStateMainMenu = 5,
	GameStateLoadingSP_MP = 6
};

class scriptHandlerMgr {
public:

	virtual ~scriptHandlerMgr();
	virtual void _Function1() = 0;
	virtual void _Function2() = 0;
	virtual void _Function3() = 0;
	virtual void _Function4() = 0;
	virtual void _Function5() = 0;
	virtual void _Function6() = 0;
	virtual void _Function7() = 0;
	virtual void _Function8() = 0;
	virtual void _Function9() = 0;
	virtual void AttachScript( scrThread * thread ) = 0;
};

class ScriptEngine {
public:

	static bool Initialize();

	static pgPtrCollection<ScriptThread> * GetThreadCollection();

	static scriptHandlerMgr * GetScriptHandleMgr();

	// Gets the active thread
	static scrThread * GetActiveThread();

	// Sets the currently running thread
	static void SetActiveThread( scrThread * thread );

	// Adds a precreated custom thread to the runtime and starts it
	static void CreateThread( ScriptThread * thread );

	// Native function handler type
	typedef void( __cdecl * NativeHandler )( scrNativeCallContext * context );

	// Gets a native function handler
	static NativeHandler GetNativeHandler( uint64_t oldHash );

	static uint64_t GetNewHashFromOldHash( uint64_t oldHash );

	static eGameState GetGameState();
};

#endif // __SCRIPT_ENGINE_H__