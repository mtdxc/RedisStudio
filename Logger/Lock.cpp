#include "stdafx.h"
#include "Lock.h"
#include <process.h>

#define VERIFY(X) X
PSyncObject::~PSyncObject()
{
	if (m_hObject != NULL)
	{
		::CloseHandle(m_hObject);
		m_hObject = NULL;
	}
}

BOOL PSyncObject::Lock(DWORD dwTimeout)
{
	DWORD dwRet = ::WaitForSingleObject(m_hObject, dwTimeout);
	if (dwRet == WAIT_OBJECT_0 || dwRet == WAIT_ABANDONED)
		return TRUE;
	else
		return FALSE;
}

BOOL PMutex::Init()
{
	__try
	{
		::InitializeCriticalSection(&m_sect);
	}
	__except(STATUS_NO_MEMORY == GetExceptionCode())
	{
		return FALSE;
	}

	return TRUE;
}

PMutex::PMutex()
{ Init(); }

PMutex::~PMutex()
{ ::DeleteCriticalSection(&m_sect); }

PMutex::operator CRITICAL_SECTION*()
{ return (CRITICAL_SECTION*) &m_sect; }

BOOL PMutex::Lock()
{	
	::EnterCriticalSection(&m_sect); 
	return TRUE; 
}

BOOL PMutex::Lock(DWORD dwTimeout)
{ ASSERT(dwTimeout == INFINITE); return Lock(); }
BOOL PMutex::Unlock()
{ ::LeaveCriticalSection(&m_sect); return TRUE; }

PEvent::PEvent(BOOL bInitiallyOwn, BOOL bManualReset, LPCTSTR pstrName,
			   LPSECURITY_ATTRIBUTES lpsaAttribute)
{
	m_hObject = ::CreateEvent(lpsaAttribute, bManualReset,
		bInitiallyOwn, pstrName);
	//if (m_hObject == NULL) AfxThrowResourceException();
}

PEvent::~PEvent()
{
}

BOOL PEvent::Unlock()
{
	return TRUE;
}

BOOL PEvent::SetEvent()
{ ASSERT(m_hObject != NULL); return ::SetEvent(m_hObject); }

BOOL PEvent::PulseEvent()
{ ASSERT(m_hObject != NULL); return ::PulseEvent(m_hObject); }

BOOL PEvent::ResetEvent()
{ ASSERT(m_hObject != NULL); return ::ResetEvent(m_hObject); }

// CWinThread
PThread::operator HANDLE() const
{ return this == NULL ? NULL : m_hThread; }

BOOL PThread::SetPriority(int nPriority)
{ ASSERT(m_hThread != NULL); return ::SetThreadPriority(m_hThread, nPriority); }

int PThread::GetPriority()
{ ASSERT(m_hThread != NULL); return ::GetThreadPriority(m_hThread); }

DWORD PThread::Resume()
{ ASSERT(m_hThread != NULL); return ::ResumeThread(m_hThread); }

DWORD PThread::Suspend()
{ ASSERT(m_hThread != NULL); return ::SuspendThread(m_hThread); }

BOOL PThread::PostMessage(UINT message, WPARAM wParam, LPARAM lParam)
{ ASSERT(m_hThread != NULL); return ::PostThreadMessage(m_nThreadID, message, wParam, lParam); }

PThread::PThread(UINT nStackSize, BOOL bAutoDel, DWORD dwFlag)
{
	m_hThread = NULL;
	m_nThreadID = 0;
	m_bAutoDelete = bAutoDel;
	CreateThread(nStackSize, dwFlag);
}

PThread::PThread()
{
	m_hThread = NULL;
	m_nThreadID = 0;
	m_bAutoDelete = TRUE;
}

#ifdef _MT
struct _THREAD_STARTUP
{
	// following are "in" parameters to thread startup
	PThread* pThread;    // CWinThread for new thread
	DWORD dwCreateFlags; // thread creation flags

	HANDLE hEvent;       // event triggered after success/non-success
	HANDLE hEvent2;      // event triggered after thread is resumed

	// strictly "out" -- set after hEvent is triggered
	BOOL bError;    // TRUE if error during startup
};

UINT APIENTRY _ThreadEntry(void* pParam)
{
	_THREAD_STARTUP* pStartup = (_THREAD_STARTUP*)pParam;
	ASSERT(pStartup != NULL);
	ASSERT(pStartup->pThread != NULL);
	ASSERT(pStartup->hEvent != NULL);
	ASSERT(!pStartup->bError);

	// pStartup is invlaid after the following
	// SetEvent (but hEvent2 is valid)
	HANDLE hEvent2 = pStartup->hEvent2;

	// allow the creating thread to return from CWinThread::CreateThread
	VERIFY(::SetEvent(pStartup->hEvent));

	// wait for thread to be resumed
	VERIFY(::WaitForSingleObject(hEvent2, INFINITE) == WAIT_OBJECT_0);
	::CloseHandle(hEvent2);

	PThread* pThread = pStartup->pThread; 
	if(pThread)
	{
		pThread->Run();
		pThread->Delete();
	}	
	return 0;   // not reached
}

#endif

BOOL PThread::CreateThread(DWORD dwCreateFlags, UINT nStackSize,
							  LPSECURITY_ATTRIBUTES lpSecurityAttrs)
{
#ifndef _MT
	dwCreateFlags;
	nStackSize;
	lpSecurityAttrs;

	return FALSE;
#else
	if(m_hThread != NULL)  // already created?
		return FALSE;
	// setup startup structure for thread initialization
	_THREAD_STARTUP startup; 
	memset(&startup, 0, sizeof(startup));
	startup.pThread = this;
	startup.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	startup.hEvent2 = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	startup.dwCreateFlags = dwCreateFlags;
	if (startup.hEvent == NULL || startup.hEvent2 == NULL)
	{
		TRACE(traceAppMsg, 0, "Warning: CreateEvent failed in CWinThread::CreateThread.\n");
		if (startup.hEvent != NULL)
			::CloseHandle(startup.hEvent);
		if (startup.hEvent2 != NULL)
			::CloseHandle(startup.hEvent2);
		return FALSE;
	}

	// create the thread (it may or may not start to run)
	m_hThread = (HANDLE)(ULONG_PTR)_beginthreadex(lpSecurityAttrs, nStackSize,  
		&_ThreadEntry, &startup, dwCreateFlags | CREATE_SUSPENDED, (UINT*)&m_nThreadID);
	if (m_hThread == NULL)
		return FALSE;

	// start the thread just for MFC initialization
	VERIFY(Resume() != (DWORD)-1);
	VERIFY(::WaitForSingleObject(startup.hEvent, INFINITE) == WAIT_OBJECT_0);
	::CloseHandle(startup.hEvent);

	// if created suspended, suspend it until resume thread wakes it up
	if (dwCreateFlags & CREATE_SUSPENDED)
		VERIFY(::SuspendThread(m_hThread) != (DWORD)-1);

	// if error during startup, shut things down
	if (startup.bError)
	{
		VERIFY(::WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0);
		::CloseHandle(m_hThread);
		m_hThread = NULL;
		::CloseHandle(startup.hEvent2);
		return FALSE;
	}

	// allow thread to continue, once resumed (it may already be resumed)
	::SetEvent(startup.hEvent2);
	return TRUE;
#endif //!_MT
}

void PThread::SetNoAutoDelete()
{m_bAutoDelete = FALSE;}

BOOL PThread::WaitClose(DWORD tick)
{
	if(m_hThread)
		return WAIT_OBJECT_0 == WaitForSingleObject(m_hThread,tick);
	return TRUE;
}

void PThread::Delete()
{
	// delete thread if it is auto-deleting
	if (m_bAutoDelete)
		delete this;
}
