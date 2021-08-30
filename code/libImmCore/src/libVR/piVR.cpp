//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include "piVR.h"
#include "oculus/piOculus.h"
#include "vive/piVive.h"

namespace ImmCore
{
	piVRHMD * piVRHMD::Create(HwType hw, const char * appID, int deviceID, float pixelDensity, piLog* log, piTimer* timer)
	{
        if (hw == Oculus_Rift || hw == ANY_AVAILABLE)
		{
			piVROculus * me = new piVROculus();
			if (me->Init(appID, deviceID, pixelDensity, log, timer))
				return me;
		}
#if 0
		if (hw == HTC_Vive || hw == ANY_AVAILABLE)
		{
			piVRVive *me = new piVRVive();
			if (me->Init(appID, deviceID, pixelDensity))
				return me;
		}
#endif
		return nullptr;
	}

	void piVRHMD::Destroy(piVRHMD * me)
	{
		me->DeInit();
		delete me;
	}

    void piVRHMD::AttachSoundEngine(piSoundEngine * engine)
    {
        mSoundEngine = engine;
    }
/*
    void piVRHMD::SetPacketHandler(PacketHandler handler)
    {
        mNetwork.mOnPeerPacket = handler;
    }
    void piVRHMD::SetUserConnectHandler(UserHandler handler)
    {
        mNetwork.mOnUserConnect = handler;
    }
    void piVRHMD::SetUserLeaveHandler(UserHandler handler)
    {
        mNetwork.mOnUserLeave = handler;
    }
*/
}
