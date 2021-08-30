#include "../piLog.h"

#include <cwchar>
#include <string>
#include <cstdlib>

#include <android/log.h>

#include <chrono>

#include "piCStr.h"

namespace ImmCore {

piLog::piLog()
{
}

piLog::~piLog()
{
}

bool piLog::Init( const wchar_t *path, int loggers)
{
	return true;
}

void piLog::End( void )
{
}

void piLog::Printf( const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *format, ... )
{
	wchar_t output[1000];

	// Apparently MSVCRT has %s and %ls backwards; lets fix the format string
	std::wstring formatStr(format);
	replaceAll(formatStr, L"%s", L"%ls");

	va_list arglist;
	va_start( arglist, format );
	int len = vswprintf(output, 1000, formatStr.c_str(), arglist);

	char outputStr[len+1];
	std::wcstombs(outputStr, output, len);
	outputStr[len] = '\0';

	int logLevel = ANDROID_LOG_DEBUG;
	if (type == 1)
	{
		logLevel = ANDROID_LOG_ERROR;
	}
	else if (type == 2)
	{
		logLevel = ANDROID_LOG_WARN;
	}
	else if (type == 4)
	{
		logLevel = ANDROID_LOG_INFO;
	}

	__android_log_print(logLevel, "piLog", "%s", outputStr);

	va_end( arglist );
}

}
