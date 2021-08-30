#pragma once

#include "../../libBasics/piTArray.h"
#include "../../libBasics/piFile.h"
#include "../piWave.h"

namespace ImmCore
{
    bool  WriteOPUSToMemory(piTArray<uint8_t> *data, const piWav *me, int bitRate);
    bool WriteOPUSToFile(const wchar_t * name, const piWav *wav, int bitRate);
}
