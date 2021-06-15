#pragma once

#include <functional>

#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piStreamO.h"
#include "toImmersiveLayerSound.h"
#include "../document/sequence.h"

namespace ImmExporter
{
    bool ExportToFile(const char* iFileName, ImmExporter::Sequence* Sequence, int iOpusBitRate, ImmExporter::tiLayerSound::AudioType iAudioType, std::function<void(float)> onProgress = nullptr);

    bool ExportToMemory(ImmCore::piTArray<uint8_t>* buffer, ImmExporter::Sequence* Sequence, int iOpusBitRate, ImmExporter::tiLayerSound::AudioType iAudioType, ImmCore::piLog* log, std::function<void(float)> onProgress = nullptr);

}
