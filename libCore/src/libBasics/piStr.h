//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include <stdarg.h>
#include "piTypes.h"

namespace ImmCore
{

	char *pistrtok(char *strToken, const char *strDelimit, char **context);
	void  pistrncpy(char *strDest, size_t sizeInBytes, const char *strSource, size_t count);
	void  pistrcpy(char *strDestination, size_t sizeInBytes, const char *strSource);
	int   pistrcmp(const char *stra, const char *strb);
	void  pistrncat(char *strDest, size_t bufferSizeInBytes, const char *strSource, size_t count);
	int   pisprintf(char *buffer, size_t sizeOfBuffer, const char *format, ...);
	int   pivsprintf(char *buffer, size_t sizeInBytes, const char *format, va_list arglist);
	int   pistrlen(const char *buffer);
	char *pistrconcat(const char *stra, const char *strb);

	wchar_t *       piwstrtok(wchar_t *strToken, const wchar_t *strDelimit, wchar_t **context);
	void            piwstrncpy(wchar_t *strDest, size_t sizeInBytes, const wchar_t *strSource, size_t count);
	void            piwstrcpy(wchar_t *strDestination, size_t sizeInBytes, const wchar_t *strSource);
	void            piwstrncat(wchar_t *strDest, size_t bufferSizeInBytes, const wchar_t *strSource, size_t count);
	void            piwstrcat(wchar_t *strDest, size_t bufferSizeInBytes, const wchar_t *strSource);
	int             piwsprintf(wchar_t *buffer, size_t sizeOfBuffer, const wchar_t *format, ...);
	int             pivwsprintf(wchar_t *buffer, size_t sizeInBytes, const wchar_t *format, va_list arglist);
	int             pivscwprintf(const wchar_t *format, va_list arglist);
	int             piwstrlen(const wchar_t *buffer);
	int             piwstrcmp(const wchar_t *a, const wchar_t *b);
	const wchar_t * piwcsrchr(const wchar_t *str, wchar_t c);
	int             piwstr2int(const wchar_t *str, bool *status);
	uint64_t        piwstr2uint64(const wchar_t *str, bool *status);
    void            piuint642wstr(wchar_t *dst, uint64_t n);
    int             piwstrcspn(const wchar_t *str, const wchar_t *charSet);
	wchar_t        *piwstrdup(const wchar_t *str, bool *status);
	bool            piwstrequ(const wchar_t *a, const wchar_t *b);
    bool            piwstrequnocase(const wchar_t *a, const wchar_t *b);
    void            piwstrtoupper(wchar_t *str);
    bool            piwstrcontains(const wchar_t *str, const wchar_t *what);


	wchar_t * pistr2ws(const char *ori);
    bool      pistr2ws(wchar_t *dst, const int dstLen, const char *ori);
    char    * piws2str(const wchar_t *ori);

}
