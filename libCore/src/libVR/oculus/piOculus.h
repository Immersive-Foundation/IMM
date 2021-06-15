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
		#ifdef PROVIDE_DEPTH
		ovrLayerEyeFovDepth mLayer;
		#else
		ovrLayerEyeFov      mLayer;
		#endif
		bool                mIsVisible;
		ovrHmdDesc          mHmdDesc;
		ovrSession          mSession;
		uint64_t            mFrame;
		ovrTextureSwapChain mTextureChainColor[2];
		#ifdef PROVIDE_DEPTH
		ovrTextureSwapChain mTextureChainDepth[2];
		#endif
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

        void CreateAndJoinPrivateRoom(void);
        void LaunchInviteUsersFlow();      
        void SendPacketToUser(uint64_t userID, const void* data, size_t length, bool useTCP);
        void SendPacketToRoom(const void* data, size_t length, bool useTCP);

        bool LogAllEvents(std::string eventName,  std::vector<std::map<std::string, std::string>> eventBatch);
       
	private:
        void iProcessOVRMessages();
        void iProcessNetPackets();
        

		void iHaptic(int id, float frequency, float amplitude, float duration);

		void iInitValues(void);
        
        void iProcessVoipPeerConnect(ovrMessage * message);
        void iProcessVoipStateChange(ovrMessage * message);
        void iStartVoipSoundBuffer(uint64_t userID);
        void iStopVoipSoundBuffer(uint64_t userID);
        
        void iProcessLoggedIn(ovrMessage *  message);
        void iProcessAccessToken(ovrMessage *  message);
        void iProcessEntitlement(ovrMessage * message);

        void iProcessLaunchIntentChanged(ovrMessage* message);
        void iProcessApplicationVersion(ovrMessage* message);
        
        void iProcessCreateRoomResponse(ovrMessage * message);
        void iProcessJoinRoomResponse(ovrMessage * message);
        void iProcessInviteReceived(ovrMessage * message);
        void iProcessInvitesReceived(ovrMessage * message);
        void iProcessInviteAccepted(ovrMessage * message);
        void iProcessLeaveRoomResponse(ovrMessage * message);
        void iProcessUpdateRoom(ovrMessage * message);
        
        void iProcessNetworkingPeerConnect(ovrMessage * message);
        void iProcessNetworkingStateChange(ovrMessage * message);


	};


}
