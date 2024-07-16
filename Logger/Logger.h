#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <string>
#include <strstream>
//#include "singleton.h"
long msNow();
using std::ostream;
/// 取得应用程序的安装路径
std::string AppPath();
class Logger//:public Singleton<Logger>
{
public:
	Logger();
	ostream& Begin();
	ostream& End(ostream& stm);
};

/**
* @brief Trace日志调试封装类.
*
* 用于调试输出函数调用所占用时间
*/
class LogTrace
{
public:
	LogTrace();
	LogTrace(const char* strModule, const char* strLog, unsigned long exp=5000);
	~LogTrace();

private:
	unsigned long	m_nTickCount;
	std::string		m_strModule;
	std::string		m_strLog;
	unsigned long	m_msExpire;
};

/**
* @brief 输出调试信息
*
* @param module 模块名
* @param format 格式字符串
*/
void OutputDebugInfo(const char* module, const char* format, ...);

/**
* @brief 16进制log输出函数.
*
* @param module 模块id
* @param buffer 数据区
* @param length 数据区长度
* @param format 格式化字串
* @param ...  变参
* @return 无
*/
void OutputHex(const char* module, const void* buffer, unsigned int length, const char* format = "", ...);

/**
* @brief 16进制+ANSCII log输出函数.
*
* @param module 模块id
* @param buffer 数据区
* @param length 数据区长度
* @param format 格式化字串
* @param ... 变参
* @return 无
*/
void OutputHex2(const char* module, const void* buffer, unsigned int length, const char* format, ...);

/*
So a typical usage would be:
\verbatim
ostream & s = PTrace::Begin(3, __FILE__, __LINE__);
s << "hello";
if (want_there)
s << " there";
s << '!' << PTrace::End();
\endverbatim
*/
std::ostream & LogBegin(
					  //int level,
					   const char * module,         ///< Log level for output
					   const char * fileName,  ///< Filename of source file being traced
					   int lineNum             ///< Line number of source file being traced.
					   );

/** End a trace output.
If the trace stream output is used outside of the provided macros, the
#PEndTrace function must be used at the end of the section of trace
output. A mutex is obtained on the call to #PBeginTrace which will prevent
any other threads from using the trace stream until the PEndTrace. The
#PEndTrace is used in a similar manner to #::endl or #::flush.

 So a typical usage would be:
 \verbatim
 ostream & s = PTrace::Begin();
 s << "hello";
 if (want_there)
 s << " there";
 s << '!' << PTrace::End();
 \endverbatim
*/
std::ostream & LogEnd(std::ostream & strm /** Trace output stream being completed */);

/** Output trace.
This macro outputs a trace of any information needed, using standard stream
output operators. The output is only made if the trace level set by the
#PSetTraceLevel function is greater than or equal to the #level argument.
*/
#define DBG_STM(module, args) \
	LogBegin(module, __FILE__, __LINE__) << args << LogEnd;

/** Output trace on condition.
This macro outputs a trace of any information needed, using standard stream
output operators. The output is only made if the trace level set by the
#PSetTraceLevel function is greater than or equal to the #level argument
and the conditional is PTrue. Note the conditional is only evaluated if the
trace level is sufficient.
*/
#define DBGSTM_IF(module, cond, args) \
	if(cond) LogBegin(module, __FILE__, __LINE__) << args << LogEnd;

#define	DBG_INFO	OutputDebugInfo
//demo: DBG_INFO("module_name","module_log");
#define DBG_TRACE	LogTrace ____
//demo: DBG_TRACE("module_name, "module_log");
#define DBG_HEX		OutputHex
#define DBG_HEX2	OutputHex2
//demo: DBG_INFO("module_name", buffer, buffer_length, "module_log");
#endif //__LOGGER_H__
