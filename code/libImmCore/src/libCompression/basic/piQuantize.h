//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "../../libBasics/piVecTypes.h"

namespace ImmCore
{

	namespace piQuantize
	{
		inline int   bits7( const float & v) { return int(0.5f +   127.0f*v); }
		inline ivec2 bits7( const vec2  & v) { return f2i(0.5f +   127.0f*v); }
		inline ivec3 bits7( const vec3  & v) { return f2i(0.5f +   127.0f*v); }
		inline int   bits8( const float & v) { return int(0.5f +   255.0f*v); }
		inline ivec2 bits8( const vec2  & v) { return f2i(0.5f +   255.0f*v); }
		inline ivec3 bits8( const vec3  & v) { return f2i(0.5f +   255.0f*v); }
		inline ivec2 bits9( const vec2  & v) { return f2i(0.5f +   511.0f*v); }
		inline ivec3 bits9( const vec3  & v) { return f2i(0.5f +   511.0f*v); }
		inline ivec2 bits10(const vec2  & v) { return f2i(0.5f +  1023.0f*v); }
		inline ivec3 bits10(const vec3  & v) { return f2i(0.5f +  1023.0f*v); }
		inline ivec2 bits11(const vec2  & v) { return f2i(0.5f +  2047.0f*v); }
		inline ivec3 bits11(const vec3  & v) { return f2i(0.5f +  2047.0f*v); }
		inline int   bits12(const float & v) { return int(0.5f +  4095.0f*v); }
		inline ivec2 bits12(const vec2  & v) { return f2i(0.5f +  4095.0f*v); }
		inline ivec3 bits12(const vec3  & v) { return f2i(0.5f +  4095.0f*v); }
		inline int   bits13(const float & v) { return int(0.5f +  8191.0f*v); }
		inline ivec2 bits13(const vec2  & v) { return f2i(0.5f +  8191.0f*v); }
		inline ivec3 bits13(const vec3  & v) { return f2i(0.5f +  8191.0f*v); }
		inline int   bits14(const float & v) { return int(0.5f + 16383.0f*v); }
		inline ivec2 bits14(const vec2  & v) { return f2i(0.5f + 16383.0f*v); }
		inline ivec3 bits14(const vec3  & v) { return f2i(0.5f + 16383.0f*v); }
		inline int   bits15(const float & v) { return int(0.5f + 32767.0f*v); }
		inline ivec2 bits15(const vec2  & v) { return f2i(0.5f + 32767.0f*v); }
		inline ivec3 bits15(const vec3  & v) { return f2i(0.5f + 32767.0f*v); }
		inline int   bits16(const float & v) { return int(0.5f + 65535.0f*v); }
		inline ivec3 bits16(const vec3  & v) { return f2i(0.5f + 65535.0f*v); }
		inline ivec3 bits20(const vec3  & v) { return f2i(0.5f + 1048575.0f*v); }
		inline int   bits23(const float & v) { return int(0.5f + 8388607.0f*v); }
		inline ivec2 bits23(const vec2  & v) { return f2i(0.5f + 8388607.0f*v); }
		inline ivec3 bits23(const vec3  & v) { return f2i(0.5f + 8388607.0f*v); }

		inline float ibits7( const int   & v) { return float(v) /   127.0f; }
		inline vec2  ibits7( const ivec2 & v) { return i2f(v)   /   127.0f; }
		inline vec3  ibits7( const ivec3 & v) { return i2f(v)   /   127.0f; }
		inline float ibits8( const int   & v) { return float(v) /   255.0f; }
		inline vec2  ibits8( const ivec2 & v) { return i2f(v)   /   255.0f; }
		inline vec3  ibits8( const ivec3 & v) { return i2f(v)   /   255.0f; }
		inline float ibits9( const int   & v) { return float(v) /   511.0f; }
		inline vec2  ibits9( const ivec2 & v) { return i2f(v)   /   511.0f; }
		inline vec3  ibits9( const ivec3 & v) { return i2f(v)   /   511.0f; }
		inline float ibits10(const int   & v) { return float(v) /  1023.0f; }
		inline vec2  ibits10(const ivec2 & v) { return i2f(v)   /  1023.0f; }
		inline vec3  ibits10(const ivec3 & v) { return i2f(v)   /  1023.0f; }
		inline float ibits11(const int   & v) { return float(v) /  2047.0f; }
		inline vec2  ibits11(const ivec2 & v) { return i2f(v)   /  2047.0f; }
		inline vec3  ibits11(const ivec3 & v) { return i2f(v)   /  2047.0f; }
		inline float ibits12(const int   & v) { return float(v) /  4095.0f; }
		inline vec2  ibits12(const ivec2 & v) { return i2f(v)   /  4095.0f; }
		inline vec3  ibits12(const ivec3 & v) { return i2f(v)   /  4095.0f; }
		inline float ibits13(const int   & v) { return float(v) /  8191.0f; }
		inline vec2  ibits13(const ivec2 & v) { return i2f(v)   /  8191.0f; }
		inline vec3  ibits13(const ivec3 & v) { return i2f(v)   /  8191.0f; }
		inline float ibits14(const int   & v) { return float(v) / 16383.0f; }
		inline vec2  ibits14(const ivec2 & v) { return i2f(v)   / 16383.0f; }
		inline vec3  ibits14(const ivec3 & v) { return i2f(v)   / 16383.0f; }
		inline float ibits15(const int   & v) { return float(v) / 32767.0f; }
		inline vec2  ibits15(const ivec2 & v) { return i2f(v)   / 32767.0f; }
		inline vec3  ibits15(const ivec3 & v) { return i2f(v)   / 32767.0f; }
		inline float ibits16(const int   & v) { return float(v) / 65535.0f; }
		inline vec2  ibits16(const ivec2 & v) { return i2f(v)   / 65535.0f; }
		inline vec3  ibits16(const ivec3 & v) { return i2f(v)   / 65535.0f; }
		inline vec3  ibits20(const ivec3 & v) { return i2f(v) / 1048575.0f; }
		inline vec3  ibits23(const ivec3 & v) { return i2f(v) / 8388607.0f; }
	}

} // namespace ImmCore
