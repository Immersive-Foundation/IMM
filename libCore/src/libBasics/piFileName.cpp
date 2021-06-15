//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <malloc.h>
#include <string.h>
#include "piStr.h"

#ifdef WINDOWS
#include <Windows.h>
#endif
namespace ImmCore {

static inline int is_slash( wchar_t c )
{
	if( c==L'/' )  return( 1 );
	if( c==L'\\' ) return( 1 );
	return( 0 );
}


wchar_t *piFileName_GetFullPath(const wchar_t* file)
{
#ifdef WINDOWS
    wchar_t* fullPath = (wchar_t*)malloc(MAX_PATH * sizeof(wchar_t));
    GetFullPathName(file, MAX_PATH, fullPath, nullptr);
    return fullPath;
#else
    return nullptr;
#endif
}

wchar_t * piFileName_ExtractUnit(const wchar_t *file)
{
    wchar_t *res = (wchar_t*)malloc(3 * sizeof(wchar_t));
    if (!res)
        return nullptr;

    res[0] = file[0];
    res[1] = file[1];
    res[2] = 0;

    return res;
}

wchar_t * piFileName_ExtractPath( const wchar_t *file )
{
	wchar_t	*res;

	// detect the last slash symbol
	int l = -1;
	for( int i=0; file[i]; i++ )
		if( is_slash( file[i] ) )
			l = i;

	// if no slash found, return "./"
	if( l==-1 )
	{
		res = (wchar_t*)malloc( 3*sizeof(wchar_t) );
		if( !res )
			return( 0 );
		res[0] = L'.';
		res[1] = L'\\';
		res[2] = L'\0';
	}
	else
	{
		res = (wchar_t*)malloc( (l+2)*sizeof(wchar_t) );
		if( !res )
			return( 0 );
		memcpy( res, file, (l+1)*sizeof(wchar_t) );
		res[l+1] = L'\0';
	}

	return( res );
}


wchar_t *piFileName_ExtractName( const wchar_t *file )
{
	wchar_t	*res;

	// detect the last slash symbol
	int l = -1;
	for( int i=0; file[i]; i++ )
		if( is_slash( file[i] ) )
			l = i;

	// if no slash found, return file
    size_t nl;
	if( l==-1 )
		{
        nl = piwstrlen(file);
		res = (wchar_t*)malloc( (nl+1)*sizeof(wchar_t) );
		if( !res )
			return( 0 );
        piwstrcpy( res, nl, file );
		}
	else
		{
        nl = piwstrlen(file)-l;
		res = (wchar_t*)malloc( (nl+1)*sizeof(wchar_t) );
		if( !res )
			return( 0 );
        piwstrcpy( res, nl, file+l+1 );
		}


	return( res );
}

wchar_t *piFileName_ExtractNameWithoutExtension( const wchar_t *file )
{
	wchar_t	*res = piFileName_ExtractName( file );
    
	int l = -1;
	for( int i=0; res[i]; i++ )
		if( res[i]=='.' )
			l = i;
    if( l!=-1 )
        res[l] = 0;

	return( res );
}

const wchar_t *piFileName_GetExtension(const wchar_t *file )
{
	int l = -1;
	for( int i=0; file[i]; i++ )
		if( file[i]=='.' && !is_slash(file[i+1]) )
			l = i;

	return( file+l );
}


}
