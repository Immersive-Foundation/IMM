#pragma once

#include "../../libBasics/piTArray.h"
#include "../../libBasics/piFile.h"
#include "../piWave.h"

namespace ImmCore
{
	bool  ReadWAVFromDisk(piWav *me, const wchar_t *name);
    bool  ReadWAVFromFile(piWav *me, piFile *fp);
    bool  ReadWAVFromMemory(piWav *dst, piTArray<uint8_t> *data);

	bool  WriteWAVToDisk(const wchar_t *name, const piWav *me);
    bool  WriteWAVToMemory(piTArray<uint8_t> *data, const piWav *me);
}
