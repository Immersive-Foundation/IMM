//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include "piSoundEngineNULL.h"
#ifdef WINDOWS
#include "windows/piSoundEngineDS.h"
#endif // WINDOWS

#include "windows/piSoundEngineAudioSDKBackend.h"

namespace ImmCore
{
	piSoundEngineBackend * piCreateSoundEngineBackend(piSoundEngineBackend::API api, piLog *log)
	{
		piSoundEngineBackend *me = nullptr;
		     if (api == piSoundEngineBackend::API::Null)           me = new piSoundEngineBackendNULL();
#ifdef WINDOWS
		else if (api == piSoundEngineBackend::API::DirectSound)    me = new piSoundEngineBackendDS();
#endif // WINDOWS
		else if (api == piSoundEngineBackend::API::DirectSoundOVR) me = new piSoundEngineAudioSDKBackend();
		else return nullptr;

		if (!me->doInit(log))
			return nullptr;

		return me;
	}

	void piDestroySoundEngineBackend(piSoundEngineBackend *me)
	{
		me->doDeinit();
		delete me;
	}

}
