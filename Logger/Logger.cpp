#include "stdafx.h"
#include "Logger.h"
#include "Lock.h"
#include "time.h"
#include <direct.h>
#include <fstream>
#include <sstream>
#include <tchar.h>
#include <map>
using namespace std;
#include <mmsystem.h>
long msNow()
{
  //return GetTickCount();
  return timeGetTime();
}
#define MAX_RETRY_TIME 90
#define MAX_LOG_LEN  8192
#define MAX_FILE_LENGTH 50*1024*1024

#define DEBUG_PREFIX ACE_TEXT("(%0.5t)%T %s: %s\n")

std::string AppPath()
{
	char tcPath[MAX_PATH] = {0};
	int nLen = GetModuleFileNameA( NULL, tcPath, MAX_PATH );
	while(nLen)
	{
		if(tcPath[nLen] == _T('\\'))
		{
			tcPath[nLen]=0;
			break;
		}
		nLen--;
	}
	return (char*)tcPath;
}

Logger::Logger()//:Singleton<Logger>("Logger")
{

}


struct ostm
{
	ostm(){module=NULL;stm=NULL;}
	ostringstream* stm;
	const char* module;
};// stm;
std::map<long,ostm> ostmap;
PMutex ostmutex;
std::ostream & LogBegin(
						//int level,
						const char * module,         ///< Log level for output
						const char * fileName,  ///< Filename of source file being traced
						int lineNum             ///< Line number of source file being traced.
						)
{
	PAutoLock al(ostmutex);
	ostm& stm = ostmap[::GetCurrentThreadId()];
	stm.module = "";
	if(!stm.stm) stm.stm = new ostringstream;
	return *stm.stm;
}

std::ostream & LogEnd(std::ostream & strm /** Trace output stream being completed */)
{
	PAutoLock al(ostmutex);
	ostm& stm = ostmap[::GetCurrentThreadId()];
	if(stm.stm)
	{
		(*stm.stm) << ends << flush;
		OutputDebugInfo(stm.module, stm.stm->str().c_str());
		stm.stm->str("");
		stm.module = NULL;
	}
	return strm;
}

LogTrace::LogTrace():m_msExpire(5000)
{
	LogTrace("UNDEFINED_MODULE", "NO_LOG");
}


/**
* @brief LogTrace构造函数
*
* @param strModule 模块名
* @param strLog 日志字符串前缀
* @param exp 超时时间（单位ms,缺省为5000）
*/
LogTrace::LogTrace(const char* strModule, const char* strLog,unsigned long exp)
{
	m_strModule = "UNDEFINED_MODULE";
	m_strLog	= "NO_LOG";
	if (NULL != strModule) 
		m_strModule = strModule;
	if (NULL != strLog)
		m_strLog	= strLog;
	
	m_nTickCount = msNow();
	
	if(exp>0)
		m_msExpire = exp;
	else
		m_msExpire = 5000;

	std::string log = "come in ";
	log += m_strLog;

	try
	{
		OutputDebugInfo(m_strModule.c_str(), log.c_str());
	}
	catch (...)
	{
	}
}

LogTrace::~LogTrace()
{
	try
	{
		stringstream ss;
		DWORD nUsed = msNow() - m_nTickCount;
		if (nUsed <= m_msExpire)
		{
			ss<<"it took "<<nUsed<<" ms, exit "<<m_strLog.c_str();
		}
		else
		{
			ss<<"it took "<<nUsed<<" ms, exit "<<m_strLog.c_str()<<". +++MORE TIME NEEDED+++";
		}
		OutputDebugInfo(m_strModule.c_str(), ss.str().c_str());
	}
	catch (...)
	{
	}
}

void LogOut(char* strLog,int nLen);
void OutputDebugInfo(const char* module, const char* format, ...)
{
	if (NULL == module|| format==NULL)
		return;

	SYSTEMTIME st;
	GetLocalTime(&st);
	char szLog[MAX_LOG_LEN]={0};
	/*
	int nLen = sprintf( szLog, "[%4d-%02d-%02d %02d:%02d:%02d,%03d LV:DEBUG MD:%s TH:%0.5d] ",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds ,
		module,::GetCurrentThreadId());
	*/
	int nLen = _snprintf( szLog, MAX_LOG_LEN, "(%0.5d) %02d:%02d:%02d:%04d %s: ",
		::GetCurrentThreadId(), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds , module);
	
	va_list vl;
	va_start(vl,format);
	//vsprintf_s(szLog+nLen,sizeof(szLog)-nLen,fmt,vl);
	int nRet = _vsnprintf(szLog+nLen,MAX_LOG_LEN-nLen-2,format,vl);
	if (nRet >=0)
	{
		nLen += nRet;
		if(szLog[nLen-1]!=_T('\n'))
			szLog[nLen++] = _T('\n');
		LogOut(szLog,nLen);
	}
	va_end(vl);
}

void OutputHex(const char* module, const void* buffer, unsigned int length, const char* format, ...)
{
	if ((NULL == buffer) || (NULL == module) || (0 == length))
	{
		return;
	}

	std::string strHex = "";

	char strFormat[1024];
	memset(strFormat, 0, sizeof(strFormat));
	va_list argp;
	va_start(argp, format);
	_vsnprintf(strFormat, sizeof(strFormat), (const char*)format, argp);
	va_end(argp);
	strHex = strFormat;

	int _hex = 15;
	for(unsigned int i = 0; i < length; i++)
	{
		_hex=(_hex+1)%16;
		if(_hex==0)
		{
			strHex += '\n';
		}

		char hex[4] = {0};
		sprintf(hex, " %02X", ((unsigned char*)buffer)[i]);
		strHex += (const char*)hex;
	}

	OutputDebugInfo( module, strHex.c_str() );
}

void OutputHex2(const char* module, const void* data, unsigned int length, const char* format, ...)
{
	std::string strHex = "";
	unsigned char* buffer = (unsigned char*)data;
	char strFormat[1024];
	memset(strFormat, 0, sizeof(strFormat));
	va_list argp;
	va_start(argp, format);
	_vsnprintf(strFormat, sizeof(strFormat), (const char*)format, argp);
	va_end(argp);
	strHex = strFormat;

	memset(strFormat,' ',72);
	strFormat[72]=0;
	int _hex = 0;
	for(unsigned int i = 0; i < length; i++)
	{
		sprintf(strFormat+_hex*3, " %02X", *(buffer + i));
		strFormat[56+_hex]=(buffer[i]>=0x20 && buffer[i]<=0x7e)?buffer[i]:'.';

		if(++_hex==16)
		{
			strFormat[_hex*3]=' ';
			strFormat[56+_hex]=0;
			strHex += '\n';
			strHex += strFormat;
			memset(strFormat,' ',72);
			strFormat[72]=0;
			_hex=0;
		}
	}
	if(_hex)
	{
		strFormat[_hex*3]=' ';
		strFormat[56+_hex]=0;
		strHex += '\n';
		strHex += strFormat;
	}
	OutputDebugInfo( module, strHex.c_str() );
}

void LogOut(char* strLog,int nLen)
{
	OutputDebugStringA(strLog);
	static long nLastTick = 0;
	static HWND hWndLogView = NULL;
	static PMutex sec;
	PAutoLock l(sec);
	long nTickCount = msNow();
	if ((nTickCount-nLastTick)>(MAX_RETRY_TIME*1000))
	{
		hWndLogView = ::FindWindow(_T("Coobol_LogView"), NULL);
	}
	if(hWndLogView)
	{
		COPYDATASTRUCT Data;
		Data.dwData = 0;
		Data.lpData = (void *)strLog;
		Data.cbData = nLen+1;
		::SendMessage(hWndLogView, WM_COPYDATA, 0, (LPARAM)&Data);
	}
	
	static std::fstream fpLog;
	if(!fpLog||fpLog.tellp() > MAX_FILE_LENGTH)
	{
		char tcPath[MAX_PATH] = {0};
		int nLen = GetModuleFileNameA( NULL, tcPath, MAX_PATH );
		while(nLen)
		{
			if(tcPath[nLen] == '\\'){
				tcPath[nLen+1] = 0;
				break;
			}
			nLen--;
		}
		if(0==nLen)
			return ;
		strcat(tcPath, "Log\\");
		nLen+= 4;
		mkdir(tcPath);
		SYSTEMTIME t; GetLocalTime(&t);
		sprintf( tcPath+nLen+1, "%0.4d%0.2d%0.2d%0.2d%0.2d%0.2d.log", 
			t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
		fpLog.open(tcPath, std::ios::in | std::ios::out | std::ios::app);
	}
	fpLog.clear();
	fpLog << strLog;
	fpLog.flush();
}
