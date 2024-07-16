#pragma once

class PSyncObject
{
	// Constructor
public:
	PSyncObject(){m_hObject = NULL;}
	~PSyncObject();
	// Attributes
public:
	operator HANDLE() const{return m_hObject;}
	HANDLE  m_hObject;

	// Operations
	virtual BOOL Lock(DWORD dwTimeout = INFINITE);
	virtual BOOL Unlock() = 0;
};

/////////////////////////////////////////////////////////////////////////////
// CCriticalSection

class PMutex : public PSyncObject
{
	// Constructor
public:
	PMutex();

	// Attributes
public:
	operator CRITICAL_SECTION*();
	CRITICAL_SECTION m_sect;

	// Operations
public:
	BOOL Unlock();
	BOOL Lock();
	BOOL Lock(DWORD dwTimeout);

	// Implementation
public:
	virtual ~PMutex();

private:
	BOOL Init();
};

/////////////////////////////////////////////////////////////////////////////
// CEvent

class PEvent : public PSyncObject
{
	// Constructor
public:
	/* explicit */ PEvent(BOOL bInitiallyOwn = FALSE, BOOL bManualReset = FALSE,
		LPCTSTR lpszNAme = NULL, LPSECURITY_ATTRIBUTES lpsaAttribute = NULL);

	// Operations
public:
	BOOL SetEvent();
	BOOL PulseEvent();
	BOOL ResetEvent();
	BOOL Unlock();

	// Implementation
public:
	virtual ~PEvent();
protected:
	HANDLE m_hObject;
};


class PAutoLock
{
public:
	PAutoLock(PSyncObject* cs):m_cs(*cs),bLock(FALSE){Lock();}
	PAutoLock(PSyncObject& cs):m_cs(cs),bLock(FALSE){Lock();}
	BOOL Unlock()
	{
		if(bLock&&m_cs.Unlock()) 
		{
			bLock = FALSE;
			return TRUE;
		}
		return FALSE;
	}
	BOOL Lock()
	{
		if(!bLock && m_cs.Lock())
			bLock = TRUE;
		return bLock;
	}
	~PAutoLock(){m_cs.Unlock();}
protected:
	BOOL bLock;
	PSyncObject& m_cs;
};

class PThread
{
	PThread();

	BOOL CreateThread(DWORD dwCreateFlags = 0, UINT nStackSize = 0,
		LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL);

	BOOL m_bAutoDelete;     ///< enables 'delete this' after thread termination

	// only valid while running
	HANDLE m_hThread;       ///< this thread's HANDLE
	operator HANDLE() const;
	DWORD m_nThreadID;      ///< this thread's ID
public:
	PThread(UINT nStackSize, BOOL bAutoDel, DWORD dwFlag = CREATE_SUSPENDED);
	
	void SetNoAutoDelete();

	BOOL WaitClose(DWORD tick);
	/// process function
	virtual void Run() = 0;
	
	int GetPriority();
	BOOL SetPriority(int nPriority);
	
	void Delete();
	// Operations
	DWORD Suspend();
	DWORD Resume();
	// 发送线程消息
	BOOL PostMessage(UINT message, WPARAM wParam, LPARAM lParam);
};