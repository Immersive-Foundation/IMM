#pragma once

#include <string>

namespace ImmCore
{ 
	// Help deal with malloc'd wchar_t / char conversions.
	struct ScopedCStr
	{
		ScopedCStr(wchar_t * ptr)
			: mPtr((void*)ptr)
		{
		}

		ScopedCStr(char * ptr)
			: mPtr((void*)ptr)
		{
		}

		~ScopedCStr()
		{
			free(mPtr);
		}

		wchar_t * w_cstr()
		{
			return (wchar_t *) mPtr;
		}

		char * c_str()
		{
			return (char *) mPtr;
		}

		void * mPtr = nullptr;
	};

	void replaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to);
}
