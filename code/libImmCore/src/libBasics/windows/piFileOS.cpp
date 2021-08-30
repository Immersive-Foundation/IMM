//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <errno.h>

#include "../piTypes.h"
#include "../piFile.h"

namespace ImmCore {

bool piFile::Open(const wchar_t *name, const wchar_t *mode)
{
    FILE *fp = nullptr;
    if( _wfopen_s(&fp, name, mode) != 0 )
        return false;

    mInternal = (void*)fp;
    return true;
}

bool piFile::Seek(uint64_t pos, SeekMode mode)
{
    int cmode = 0;
    if (mode == CURRENT) cmode = SEEK_CUR;
    if (mode == END) cmode = SEEK_END;
    if (mode == SET) cmode = SEEK_SET;

	return ( _fseeki64( (FILE*)mInternal, pos, cmode ) == 0 );
}

uint64_t piFile::Tell(void)
{
    return _ftelli64((FILE*)mInternal);
}

void piFile::Close(void)
{
    fclose((FILE*)mInternal);
}

bool piFile::Exists(const wchar_t *name)
{
    FILE  *fp = _wfopen(name, L"rb");
    if (!fp)
        return false;
    fclose(fp);
    return true;
}

uint64_t piFile::GetDiskSpace(const wchar_t *name)
{
    DWORD lpSectorsPerCluster = 0;
    DWORD lpBytesPerSector = 0;
    DWORD lpNumberOfFreeClusters = 0;
    DWORD lpTotalNumberOfClusters = 0;
        
    GetDiskFreeSpace( name, &lpSectorsPerCluster, &lpBytesPerSector, &lpNumberOfFreeClusters, &lpTotalNumberOfClusters );

    return static_cast<uint64_t>(lpNumberOfFreeClusters) *
           static_cast<uint64_t>(lpSectorsPerCluster) * 
           static_cast<uint64_t>(lpBytesPerSector);
}

bool piFile::DirectoryExists(const wchar_t *dirName_in)
{
    DWORD ftyp = GetFileAttributesW(dirName_in);
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;  //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;   // this is a directory!

    return false;    // this is not a directory!
}


bool piFile::HaveWriteAccess(const wchar_t *name)
{
    if (_waccess(name, 6) == -1)
    {
        return errno == ENOENT;
    }

    return true;
}

bool piFile::Copy(const wchar_t *dst, const wchar_t *src, bool failIfexists)
{
    if( CopyFile( src, dst, failIfexists)==0 )
    {
        int res = GetLastError();
        return false;
    }
    return true;
}

bool piFile::Rename(const wchar_t *dst, const wchar_t *src)
{
	if (MoveFile(src, dst) == 0)
	{
		int res = GetLastError();
		return false;
	}
	return true;
}

bool piFile::Delete(const wchar_t *target)
{
	if (DeleteFile(target) == 0)
	{
		int res = GetLastError();
		return false;
	}
	return true;
}

uint64_t piFile::GetLength(void)
{
    uint64_t p = _ftelli64( (FILE*)mInternal );
    _fseeki64((FILE*)mInternal, 0, SEEK_END);
    uint64_t l = _ftelli64((FILE*)mInternal);
    _fseeki64((FILE*)mInternal, p, SEEK_SET);
    return l;
}


bool piFile::DirectoryCreate( const wchar_t *name, bool failOnExists )
{
    if( ::CreateDirectory( name, NULL )==0 )
    {
        int res = GetLastError();
        if (res == ERROR_ALREADY_EXISTS )
            return !failOnExists;
        if (res == ERROR_PATH_NOT_FOUND )
            return false;
        return false;
    }
    return true;
}

bool piFile::DirectoryDelete(const wchar_t *name, bool evenIfNotEmpty)
{
	if (evenIfNotEmpty)
	{
		WIN32_FIND_DATA   fd;
		wchar_t mask[512];
		
		swprintf(mask, 512, L"%s/*.*", name);

		HANDLE h = FindFirstFile(mask, &fd);

		if(h != INVALID_HANDLE_VALUE )
		{
			for (;;)
			{
				swprintf(mask, 512, L"%s/%s", name, fd.cFileName);
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (fd.cFileName[0] != '.')
					{
						if (!DirectoryDelete(mask, evenIfNotEmpty))
							return false;
					}
				}
				else
				{
					if (DeleteFile(mask) == 0)
					{
						return false;
					}
				}

				if (!FindNextFile(h, &fd))
				{
					break;
				}
			}
		}
		FindClose(h);
	}

	if (::RemoveDirectory(name) == 0)
	{
		int res = GetLastError();  //145 = ERROR_DIR_NOT_EMPTY
		return false;

	}
	return true;
}

uint64_t piFile::GetLength(const wchar_t *filename)
{
	FILE *fp = nullptr;
	if (_wfopen_s(&fp, filename, L"rb") != 0)
		return false;

	uint64_t p = _ftelli64(fp);
	_fseeki64(fp, 0, SEEK_END);
	uint64_t l = _ftelli64(fp);
	_fseeki64(fp, p, SEEK_SET);
	fclose(fp);

	return l;
}


}
