#include <stdio.h>
#include <errno.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "piCStr.h"

#include "../piStr.h"
#include "../piTypes.h"
#include "../piFile.h"

namespace ImmCore {

bool piFile::Open(const wchar_t *name, const wchar_t *mode)
{
	ScopedCStr nameStr(piws2str(name));
	ScopedCStr modeStr(piws2str(mode));

	FILE * fp = fopen(nameStr.c_str(), modeStr.c_str());
	if(!fp)
	{
		return false;
	}

	mInternal = (void*)fp;
	return true;
}

bool piFile::Seek(uint64_t pos, SeekMode mode)
{
	int cmode = 0;
	if (mode == CURRENT) cmode = SEEK_CUR;
	if (mode == END) cmode = SEEK_END;
	if (mode == SET) cmode = SEEK_SET;

	return ( fseek( (FILE*)mInternal, pos, cmode ) == 0 );
}

uint64_t piFile::Tell(void)
{
	return ftell((FILE*)mInternal);
}

void piFile::Close(void)
{
	fclose((FILE*)mInternal);
}

bool piFile::Exists(const wchar_t *name)
{
    if (name == nullptr)
	{
    	return false;
	}

	ScopedCStr nameStr(piws2str(name));
	FILE  *fp = fopen(nameStr.c_str(), "rb");
	if (!fp)
	{
		return false;
	}
	fclose(fp);
	return true;
}

bool piFile::DirectoryExists(const wchar_t *dirName_in)
{
	if (dirName_in == nullptr)
	{
    	return false;
	}

	struct stat info;
	ScopedCStr dirNameStr(piws2str(dirName_in));
	if(stat(dirNameStr.c_str(), &info) != 0)
	{
		return false;
	}
	return S_ISDIR(info.st_mode);
}

bool piFile::HaveWriteAccess(const wchar_t *name)
{
	if (name == nullptr)
	{
    	return false;
	}

	ScopedCStr nameStr(piws2str(name));
	if (access(nameStr.c_str(), W_OK) == -1)
	{
		return errno == ENOENT;
	}

	return true;
}

bool piFile::Copy(const wchar_t *dst, const wchar_t *src, bool failIfexists)
{
	// TODO: This seems to only be used by layerPicture which we'll leave unimplemented
	return false;
}

uint64_t piFile::GetLength(void)
{
	uint64_t p = ftell( (FILE*)mInternal );
	fseek((FILE*)mInternal, 0, SEEK_END);
	uint64_t l = ftell((FILE*)mInternal);
	fseek((FILE*)mInternal, p, SEEK_SET);
	return l;
}


bool piFile::DirectoryCreate( const wchar_t *name, bool failOnExists )
{
	if (DirectoryExists(name))
	{
		return !failOnExists;
	}

	ScopedCStr nameStr(piws2str(name));
	int result = mkdir(nameStr.c_str(), 0777);
	return result != -1;
}

bool piFile::DirectoryDelete(const wchar_t *name, bool evenIfNotEmpty)
{
	if (!DirectoryExists(name))
	{
		return true;
	}

	ScopedCStr nameStr(piws2str(name));
	int result = rmdir(nameStr.c_str());
	return result != -1;
}

}
