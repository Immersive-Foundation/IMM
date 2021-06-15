#include "piCStr.h"

namespace ImmCore
{ 
	void replaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to)
	{
		if ( from.empty() )
			return;

		size_t start_pos = 0;

		while ( (start_pos = str.find(from, start_pos)) != std::wstring::npos )
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();
		}
	}
}
