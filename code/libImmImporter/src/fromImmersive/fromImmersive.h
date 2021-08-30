#pragma once

#include "libImmCore/src/libBasics/piTArray.h"
#include "libImmCore/src/libBasics/piTypes.h"
#include "../document//sequence.h"

namespace ImmImporter
{

    bool IsLoadingAsync();

    bool IsStoppedLoading();
    void StopLoadingAsync();
		
    bool ImportFromDisk(Sequence* sq, ImmCore::piLog* log, const wchar_t* filename, const Drawing::ColorSpace colorSpace, Drawing::PaintRenderingTechnique renderingTechnique);
    bool ImportFromMemory(ImmCore::piTArray<uint8_t>* data, Sequence* sq, ImmCore::piLog* log, const Drawing::ColorSpace colorSpace, Drawing::PaintRenderingTechnique renderingTechnique);
}
