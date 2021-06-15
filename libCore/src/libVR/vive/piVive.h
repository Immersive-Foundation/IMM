//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#if 0


#pragma once

#include "../piVR.h"

#include <openvr.h>

namespace ImmCore {

	class piVRVive : public piVRHMD
	{
	private:
		piVRHMD::HmdInfo    mData;
		vr::IVRSystem      *mHMD;

		bool mEnableMipmapping;
		piTimer mTimer;

	public:
		piVRVive();
		virtual ~piVRVive();

		bool Init(const char * appID, int deviceID, float pixelDensity);
		void DeInit(void);
		void GetLastError(char* errorStr);
        void GetControllersAvailable(bool *leftTouch, bool *rightTouch, bool *remote);
		bool AttachToWindow(bool createMirrorTexture, int mirrorTextureSizeX, int mirrorTextureSizeY);
		bool AttachToWindow2(void *t1, void *t2);
		void BeginFrame(int *texIndexLeft, int *texIndexRight, bool *outNeedsMipMapping);
		void EndFrame(void);
		void GetHmdState(HmdState * state);
		void *GetSoundOutputGUID(void);
		void SetTrackingOrigin(void);
		void SetTrackingOriginType(TrackingOrigin type);
		bool RecreateHeadset(void);
	};

}
#endif
