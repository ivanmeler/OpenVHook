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

	inline void* GetResultPointer() {

		return m_pReturn;
	}
};

#undef Yield

class Script {
public:

	inline Script( void( *function )( ) ) {

		scriptFiber = nullptr;
		callbackFunction = function;
		wakteAt = timeGetTime();
	}

	inline ~Script() {

		if ( scriptFiber ) {
			DeleteFiber( scriptFiber );
		}
	}

	void Tick();

	void Yield( uint32_t time );

	inline void( *GetCallbackFunction() )( ) {

		return callbackFunction;
	}

private:

	HANDLE			scriptFiber;
	uint32_t		wakteAt;
	void( *callbackFunction )( );

	void			Run();
};

typedef std::vector<std::shared_ptr<Script>> scriptVec;

class ScriptManagerThread : public ScriptThread {
private:

	scriptVec				m_scripts;

public:

	virtual void			DoRun() override;
	virtual eThreadState	Reset( uint32_t scriptHash, void * pArgs, uint32_t argCount ) override;
	void					AddScript( void( *fn )( ) );
	void					RemoveScript( void( *fn )( ) );
};

namespace ScriptManager {

	void					WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
}

extern ScriptManagerThread	g_ScriptManagerThread;

#endif // __SCRIPT_MANAGER_H__
