#include "Thread.h"

Thread::Thread(ThreadCallback callback) : Thread(callback, NULL)
{ }

Thread::Thread(ThreadCallback callback, LPVOID param) : m_handle(0)
{
	m_info.m_callback = callback;
	m_info.m_state.shouldExit = false;
	m_info.m_state.parameter = param;
}

DWORD Thread::ThreadStart(LPVOID pParam)
{
	auto info = reinterpret_cast<ThreadInfo*>(pParam);

	info->m_state.running = 1;

	do {
		if (info->m_callback) {
			info->m_callback(&info->m_state);
		}
	} while (!info->m_state.shouldExit);

	info->m_state.running = 0;

	return 0;
}

void Thread::Run(int nPriority)
{
	m_handle = CreateThread(NULL, NULL, ThreadStart, &m_info, NULL, NULL);

	SetThreadPriority(m_handle, nPriority);

	SetThreadPriorityBoost(m_handle, TRUE);

	ResumeThread(m_handle);
}

void Thread::Run()
{
	Run(THREAD_PRIORITY_NORMAL);
}

void Thread::Exit()
{
	m_info.m_state.shouldExit = true;
}

Thread::~Thread()
{
	Exit();
}
