#ifndef __SCRIPT_MANAGER_H__
#define __SCRIPT_MANAGER_H__

#include "ScriptEngine.h"
#include "NativeInvoker.h"

class ScriptManagerContext : public NativeContext {
public:

	ScriptManagerContext()
		: NativeContext() {
	}

	void Reset() {

		m_nArgCount = 0;
		m_nDataCount = 0;
	}

	void* GetResultPointer() {

		return m_pReturn;
	}
};

#undef Yield

class Script {
public:

	Script( void( *function )( ) ) {

		scriptFiber = nullptr;
		callbackFunction = function;
		wakedAt = timeGetTime();
	}

	~Script() {

		if ( scriptFiber ) {
			DeleteFiber( scriptFiber );
		}
	}

	void Tick();

	void Yield( uint32_t time );

	void( *GetCallbackFunction() )( ) {

		return callbackFunction;
	}

private:

	HANDLE			scriptFiber;
	uint32_t		wakedAt;
	void( *callbackFunction )( );

	void			Run();
};

typedef std::map<HMODULE,std::vector<std::shared_ptr<Script>>> scriptMap;

class ScriptManagerThread : public ScriptThread {

	scriptMap					m_scripts;
	std::vector<std::string>	m_scriptNames;

public:

	void			DoRun() override;
	eThreadState	Reset( uint32_t scriptHash, void * pArgs, uint32_t argCount ) override;
	bool					LoadScripts();
	void					FreeScripts();
	void					AddScript( HMODULE module, void( *fn )( ) );
	void					RemoveScript( void( *fn )( ) );
	void					RemoveScript( HMODULE module );
};

namespace ScriptManager {

	void					HandleKeyEvent(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow);
}

extern ScriptManagerThread	g_ScriptManagerThread;

#endif // __SCRIPT_MANAGER_H__
