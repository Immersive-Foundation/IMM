//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

namespace ImmCore {

// return a new string with the path of the file
wchar_t *piFileName_ExtractPath( const wchar_t *file );

// return a new string with the name of the file without the path
wchar_t *piFileName_ExtractName( const wchar_t *file );

wchar_t *piFileName_ExtractNameWithoutExtension( const wchar_t *file );

// return a pointer to the dot of the extension. Do not free this pointer
const wchar_t *piFileName_GetExtension( const wchar_t *file );

// return a new string with the unit of the file, for example, L"C:"
wchar_t *piFileName_ExtractUnit(const wchar_t *file);

// return a full file path from local path.
wchar_t *piFileName_GetFullPath(const wchar_t* file);
} // namespace ImmCore
