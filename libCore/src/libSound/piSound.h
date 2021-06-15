//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piSoundEngineBackend.h"

namespace ImmCore {

piSoundEngineBackend * piCreateSoundEngineBackend(piSoundEngineBackend::API api, piLog *log);
void                   piDestroySoundEngineBackend(piSoundEngineBackend *me);

} // namespace ImmCore
