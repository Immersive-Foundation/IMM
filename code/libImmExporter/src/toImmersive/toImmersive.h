#pragma once

#include <functional>

#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libBasics/piStreamO.h"
#include "toImmersiveLayerSound.h"
#include "../document/sequence.h"

namespace ImmExporter
{
    bool ExportToFile(const char* iFileName, ImmExporter::Sequence* Sequence, int iOpusBitRate, ImmExporter::tiLayerSound::AudioType iAudioType, std::function<void(float)> onProgress = nullptr);

    bool ExportToMemory(ImmCore::piTArray<uint8_t>* buffer, ImmExporter::Sequence* Sequence, int iOpusBitRate, ImmExporter::tiLayerSound::AudioType iAudioType, ImmCore::piLog* log, std::function<void(float)> onProgress = nullptr);

}
