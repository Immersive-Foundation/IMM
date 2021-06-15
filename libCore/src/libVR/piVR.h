//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include <functional>
#include <map>
#include <vector>
#include "../libBasics/piLog.h"
#include "../libSound/piSound.h"
#include "../libBasics/piTimer.h"
#include "../libBasics/piString.h"

namespace ImmCore {

    class piVRHMD
    {
    public:
        typedef struct
        {
            bool  mStatePressed;
            bool  mEventDown;
            bool  mEventUp;
            bool  mEventDoubleClick;
            bool  mEventSingleClick;
            int   mInternal1;
            uint64_t mInternal2;
        }Button;

        typedef struct
        {
            bool mStateTouched;
            bool mEventTouchBegin;
            bool mEventTouchEnd;
        }Touch;

        typedef struct
        {
            float mX; // Horizontal and vertical thumbstick axis values , in the range -1.0f to 1.0f.
            float mY;

            bool mEventSwipeUp;
            bool mEventSwipeDown;
            bool mEventSwipeLeft;
            bool mEventSwipeRight;

            int mInternalX;
            int mInternalY;

        }Joystick;

        typedef struct
        {
            bool  mEnabled;
            float mPosition[3];
            float mRotation[4];
            float mVelocity[3];
            float mAngularVelocity[3];

            float mIndexTrigger; // Left and right finger trigger values, in the range 0.0 to 1.0f.
            float mHandTrigger; // Left and right hand trigger values , in the range 0.0 to 1.0f.

            Joystick mThumbstick;

            Touch mUpTouch;
            Touch mDownTouch;
            Touch mThumbstickTouch;
            Touch mIndexTriggerTouch;

            Button mUpButton;
            Button mDownButton;
            Button mThumbstickButton;
            Button mMenuButton;

            //void (*Vibrate)(int id, float frequency, float amplitude, float duration);
            std::function<void(int id, float frequency, float amplitude, float duration)> Vibrate;

        }Controller;

        typedef struct
        {
            bool   mEnabled;
            Button mLeftButton;
            Button mRightButton;
            Button mUpButton;
            Button mDownButton;
            Button mEnterButton;
            Button mVolumeUpButton;
            Button mVolumeDownButton;
            Button mHomeButton;
        }Remote;

        typedef struct
        {
            float mCamera[16];
            float mProjection[4];
        }HeadInfo;

        typedef struct
        {
            int   mVP[4];
            float mProjection[4];
            float mCamera[16];
        }EyeInfo;

        typedef struct
        {
            int                 mNum;
            unsigned int        mTexIDColor[64];
#ifdef PROVIDE_DEPTH
            unsigned int        mTexIDDepth[64];
#endif
        }TextureChain;

        typedef struct
        {
            // static
            int          mVRXres;
            int          mVRYres;
            TextureChain mTexture[2];
            unsigned int mMirrorTexID;

            // per frame
            HeadInfo     mHead;
            EyeInfo      mEye[2];
            Controller   mController[2];
            Remote       mRemote;
        }HmdInfo;

        HmdInfo mInfo;

        typedef struct
        {
            bool mShouldQuit;
            bool mHMDWorn;
            bool mIsVisible;
            bool mHMDLost;
            bool mTrackerConnected;
            bool mPositionTracked;
        } HmdState;

        HmdState mState;

        typedef struct
        {
            uint64_t mUserID = 0;
            bool mLoggedIn = false;
            bool mNotEntitled = false;
			const char* mUserLocale = nullptr;
			piString mUserAccessToken;
			piString mAppVersion;
            const char* mDeepLinkMessage = nullptr;
        }
        PlatformState;

        PlatformState mPlatform;
        piTimer* mTimer;
        piSoundEngine* mSoundEngine = nullptr;
        typedef std::function<void(uint64_t, const void*, size_t)> PacketHandler;
        typedef std::function<void(uint64_t)> UserHandler;

        struct Peer
        {
            uint64_t mUserID;
            int mSoundID;
        };

#define MAX_NETWORKING_PEERS 3
    typedef struct
    {
        unsigned char mNumPeers = 0;
        uint64_t mRoomID = 0;
        PacketHandler mOnPeerPacket;
        UserHandler mOnUserConnect;
        UserHandler mOnUserLeave;
        Peer mPeers[MAX_NETWORKING_PEERS];

    } NetworkState;

    NetworkState mNetwork;

    typedef enum
    {
        FloorLevel, EyeLevel
    }
    TrackingOrigin;


    typedef enum
    {
        Oculus_Rift = 0,
        HTC_Vive = 1,
        SonyVR = 2,
		Oculus_RiftS = 3,
		Oculus_Quest = 4,
        ANY_AVAILABLE = 99
    }HwType;
	HwType mType;

    virtual bool   Init(const char * appID, int deviceID, float pixelDensity, piLog* log, piTimer* timer) = 0;
    virtual void   DeInit(void) = 0;
    virtual void   GetLastError(char * errorString) = 0;
    virtual void   GetControllersAvailable(bool *leftTouch, bool *rightTouch, bool *remote) = 0;
    virtual bool   AttachToWindow(bool createMirrorTexture, int mirrorTextureSizeX, int mirrorTextureSizeY) = 0;
    virtual bool   AttachToWindow2(void *t1, void *t2) = 0;
    virtual void   BeginFrame(int *texIndexLeft, int *texIndexRight, bool *outNeedsMipMapping) = 0;
    virtual void   EndFrame(void) = 0;

    virtual void   GetHmdState(HmdState * state) = 0;

    void AttachSoundEngine(piSoundEngine * engine);
    void SetPacketHandler(PacketHandler handler);
    void SetUserConnectHandler(UserHandler handler);
    void SetUserLeaveHandler(UserHandler handler);

	virtual void  *GetSoundOutputGUID(void) = 0;
    virtual wchar_t  *GetSoundOutputName(void) = 0;
    virtual void   SetTrackingOrigin(void) = 0;
    virtual void   SetTrackingOriginType(TrackingOrigin type) = 0;
	virtual bool   RecreateHeadset(void) = 0;

    virtual void   CreateAndJoinPrivateRoom() = 0;
    virtual void   LaunchInviteUsersFlow() = 0;

    virtual void   SendPacketToUser(uint64_t userID, const void* data, size_t length, bool useTCP) = 0;
    virtual void   SendPacketToRoom(const void* data, size_t length, bool useTCP) = 0;

    virtual bool   LogAllEvents(std::string eventName, std::vector<std::map<std::string, std::string>> eventBatch) = 0;
	static piVRHMD   *Create(HwType hw, const char * appID, int deviceID, float pixelDensity, piLog* log, piTimer* timer);
	static void       Destroy(piVRHMD * me);



};



} // namespace ImmCore
