#pragma once

#include "OpenVHook.h"

typedef LPVOID ThreadParameter;

struct ThreadState
{
	BOOL running, shouldExit;
	ThreadParameter parameter;
};

typedef void(*ThreadCallback)(ThreadState * pState);

struct ThreadInfo
{
	ThreadState m_state;
	ThreadCallback m_callback;
};

class Thread
{
	ThreadInfo m_info;
	HANDLE m_handle;
	static DWORD WINAPI ThreadStart(LPVOID pParam);

public:
	Thread(ThreadCallback callback);

	Thread(ThreadCallback callback, LPVOID param);

	void Run();

	void Run(int nPriority);

	void Exit();

	~Thread();
};

