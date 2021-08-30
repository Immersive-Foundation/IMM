//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once


#include "../piVR.h"

#include "OVR_Platform.h"
#include "OVR_CAPI_Audio.h"

namespace ImmCore
{
	class piVROculus : public piVRHMD
	{
	private:
		ovrLayerEyeFov      mLayer;
		bool                mIsVisible;
		ovrHmdDesc          mHmdDesc;
		ovrSession          mSession;
		uint64_t            mFrame;
		ovrTextureSwapChain mTextureChainColor[2];
		ovrSizei            mRecommendedTexSize[2];
        bool                mUsingMirrorTexture;
		ovrMirrorTexture    mMirrorTexture;
		bool                mEnableMipmapping;
		GUID			    mSoundGUID;
        wchar_t             mSoundName[128];
		float               mPixelDensity;
		struct HapticState
		{
			int    mState;
			float  mDuration;
			double mTime;
		}mHapticState[2];

		piTimer* mTimer;
        piLog* mLog;
		int mLogInRetries = 0;
		int mGetAccessTokenRetries = 0;
	public:
		piVROculus();
		virtual ~piVROculus();

		bool Init(const char * appID, int deviceID, float pixelDensity, piLog* log, piTimer* timer);
		void DeInit(void);
		void GetLastError(char* errorStr);
        void GetControllersAvailable(bool *left, bool *right, bool *remote);

		bool AttachToWindow(bool createMirrorTexture, int mirrorTextureSizeX, int mirrorTextureSizeY);
		bool AttachToWindow2(void *t1, void *t2);
		void BeginFrame(int *texIndexLeft, int *texIndexRight, bool *outNeedsMipMapping);
		void EndFrame(void);
        
        void   GetHmdState(HmdState * state);
        
		void *GetSoundOutputGUID(void);
		wchar_t *GetSoundOutputName(void);
		void SetTrackingOrigin(void);
		void SetTrackingOriginType(TrackingOrigin type);
		bool RecreateHeadset(void);

	private:
        
		void iHaptic(int id, float frequency, float amplitude, float duration);
		void iInitValues(void);
	};


}
