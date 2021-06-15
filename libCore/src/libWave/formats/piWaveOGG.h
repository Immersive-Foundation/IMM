#pragma once

#include "../../libBasics/piTArray.h"
#include "../../libBasics/piFile.h"
#include "../piWave.h"

namespace ImmCore {

    bool ReadOGGFromDisk(piWav *dst, const wchar_t *name);
	bool ReadOGGFromFile(piWav *dst, piFile *fp, uint64_t cropSize); // use 0xffffffffffffffff for none
	bool ReadOGGFromMemory(piWav *dst, piTArray<uint8_t> *data);

	bool WriteOGGToMemory(piTArray<uint8_t> *array, const piWav *wav);
	bool WriteOGGToFile(const wchar_t * name, const piWav *wav);


}
