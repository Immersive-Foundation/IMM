//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include "../../libBasics/piTypes.h"
#include "../../libBasics/piVecTypes.h"
#include "../../libBasics/piStr.h"
#include <vector>
#if defined(OVR_OS_WIN32) || defined(OVR_OS_WIN64)
#include <windows.h>
#endif

#include "OVR_Platform_Internal.h"
#include "OVR_CAPI_GL.h"
//#include "OVR_CAPI_D3D.h" - iq TODO: enable DX

#include <string.h>

#include "piOculus.h"

namespace ImmCore
{
    static int imax(int a, int b) { return (a > b) ? a : b; }

    static void iTouchInit(piVRHMD::Touch *b)
    {
        b->mStateTouched = false;
        b->mEventTouchBegin = false;
        b->mEventTouchEnd = false;
    }

    static void iTouchLogic(piVRHMD::Touch *b, bool state)
    {
        bool oldState = b->mStateTouched;
        b->mStateTouched = state;
        b->mEventTouchBegin = (!oldState &&  b->mStateTouched);
        b->mEventTouchEnd = (oldState && !b->mStateTouched);
    }


    static void iButtonInit(piVRHMD::Button *b)
    {
        b->mInternal1 = 0;
        b->mInternal2 = 0;
        b->mStatePressed = false;
        b->mEventDown = false;
        b->mEventUp = false;
        b->mEventDoubleClick = false;
        b->mEventSingleClick = false;
    }

    static void iJoystickInit(piVRHMD::Joystick *j)
    {
        j->mEventSwipeDown = false;
        j->mEventSwipeLeft = false;
        j->mEventSwipeRight = false;
        j->mEventSwipeUp = false;
    }

    static void iJoystickLogic(piVRHMD::Joystick *j, float x, float y)
    {
        // X -------------------------------
        j->mX = x;
        j->mEventSwipeLeft = false;
        j->mEventSwipeRight = false;
        if (x > 0.8f)
        {
            if (j->mInternalX != 1)
            {
                j->mEventSwipeRight = true;
                j->mInternalX = 1;
            }
        }
        else if (x < -0.8f)
        {
            if ((j->mInternalX != -1))
            {
                j->mEventSwipeLeft = true;
                j->mInternalX = -1;
            }
        }
        else {
            j->mInternalX = 0;
        }


        // Y -------------------------------
        j->mY = y;
        j->mEventSwipeUp = false;
        j->mEventSwipeDown = false;

        if (y > 0.8f)
        {
            if (j->mInternalY != 1)
            {
                j->mEventSwipeUp = true;
                j->mInternalY = 1;
            }
        }
        else if (y < -0.8f)
        {
            if (j->mInternalY != -1)
            {
                j->mEventSwipeDown = true;
                j->mInternalY = -1;
            }
        }
        else
        {
            j->mInternalY = 0;
        }
    }

    static void iButtonLogic(piVRHMD::Button *b, bool state, uint64_t time)
    {
        bool oldUpButton = b->mStatePressed;
        b->mStatePressed = state;
        b->mEventDown = (!oldUpButton &&  b->mStatePressed);
        b->mEventUp = (oldUpButton && !b->mStatePressed);

        // pick double click
        b->mEventDoubleClick = false;
        b->mEventSingleClick = false;
        if (b->mInternal1 == 0)
        {
            if (b->mEventDown) // single CLICK
            {
                b->mInternal2 = time;
                b->mInternal1 = 1;
            }
        }
        else if (b->mInternal1 == 1)
        {
            if ((time - b->mInternal2) > 250)
            {
                b->mInternal1 = 0;
                b->mEventSingleClick = true;
            }
            else if (b->mEventDown) // double Y click
            {
                b->mInternal1 = 0;
                b->mEventDoubleClick = true;
            }
        }

    }

    //--------------------------

    piVROculus::piVROculus()
    {
        memset(&mInfo, 0, sizeof(mInfo));
        memset(&mState, 0, sizeof(mState));
        memset(&mLayer, 0, sizeof(mLayer));
        mIsVisible = false;
        memset(&mHmdDesc, 0, sizeof(mHmdDesc));
        memset(&mSession, 0, sizeof(mSession));
        mFrame = 0;
        memset(&mTextureChainColor, 0, sizeof(mTextureChainColor));
#ifdef PROVIDE_DEPTH
        memset(&mTextureChainDepth, 0, sizeof(mTextureChainDepth));
#endif
        memset(&mRecommendedTexSize, 0, sizeof(mRecommendedTexSize));
        memset(&mMirrorTexture, 0, sizeof(mMirrorTexture));
        mEnableMipmapping = false;
        memset(&mSoundGUID, 0, sizeof(mSoundGUID));
        mSoundName[0] = 0;
    }

    piVROculus::~piVROculus()
    {
    }




    void piVROculus::iInitValues(void)
    {
        mFrame = 0;

        mInfo.mVRXres = mRecommendedTexSize[0].w;
        mInfo.mVRYres = mRecommendedTexSize[0].h;

        for (int i = 0; i < 2; i++)
        {
            iButtonInit(&mInfo.mController[i].mUpButton);
            iButtonInit(&mInfo.mController[i].mDownButton);
            iButtonInit(&mInfo.mController[i].mThumbstickButton);
            iButtonInit(&mInfo.mController[i].mMenuButton);

            iTouchInit(&mInfo.mController[i].mUpTouch);
            iTouchInit(&mInfo.mController[i].mDownTouch);
            iTouchInit(&mInfo.mController[i].mIndexTriggerTouch);
            iTouchInit(&mInfo.mController[i].mThumbstickTouch);

            iJoystickInit(&mInfo.mController[i].mThumbstick);

            mInfo.mController[i].Vibrate = [&](int id, float frequency, float amplitude, float duration)
            {
                iHaptic(id, frequency, amplitude, duration);
            };
            mHapticState[i].mState = 0;
        }

        iButtonInit(&mInfo.mRemote.mDownButton);
        iButtonInit(&mInfo.mRemote.mUpButton);
        iButtonInit(&mInfo.mRemote.mLeftButton);
        iButtonInit(&mInfo.mRemote.mRightButton);
        iButtonInit(&mInfo.mRemote.mVolumeUpButton);
        iButtonInit(&mInfo.mRemote.mVolumeDownButton);
        iButtonInit(&mInfo.mRemote.mHomeButton);


        mState.mHMDLost = false;
        mState.mIsVisible = false;
        mState.mShouldQuit = false;
        mState.mHMDWorn = false;
        mState.mPositionTracked = false;
        mState.mTrackerConnected = false;
    }

    bool piVROculus::RecreateHeadset(void)
    {
        if (mUsingMirrorTexture) ovr_DestroyMirrorTexture(mSession, mMirrorTexture);
        ovr_Destroy(mSession);
        ovr_Shutdown();

        memset(&mInfo, 0, sizeof(mInfo));
        memset(&mState, 0, sizeof(mState));
        memset(&mLayer, 0, sizeof(mLayer));
        memset(&mHmdDesc, 0, sizeof(mHmdDesc));
        memset(&mSession, 0, sizeof(mSession));
        mFrame = 0;
        memset(&mTextureChainColor, 0, sizeof(mTextureChainColor));
#ifdef PROVIDE_DEPTH
        memset(&mTextureChainDepth, 0, sizeof(mTextureChainDepth));
#endif
        memset(&mRecommendedTexSize, 0, sizeof(mRecommendedTexSize));
        memset(&mMirrorTexture, 0, sizeof(mMirrorTexture));
        memset(&mSoundGUID, 0, sizeof(mSoundGUID));
        mSoundName[0] = 0;


        ovrGraphicsLuid luid;
        if (ovr_Initialize(nullptr) < 0)
            return false;

        ovrResult res = ovr_Create(&mSession, &luid);
        if (!OVR_SUCCESS(res))
            return false;

        res = ovr_GetAudioDeviceOutGuid(&mSoundGUID);
        res = ovr_GetAudioDeviceOutGuidStr(mSoundName);

        mHmdDesc = ovr_GetHmdDesc(mSession);
		if (mHmdDesc.Type <= ovrHmd_CV1)
			mType = piVRHMD::Oculus_Rift;
		else if (mHmdDesc.Type == ovrHmd_RiftS)
			mType = piVRHMD::Oculus_RiftS;
		else
			mType = piVRHMD::Oculus_Quest;

        for (int i = 0; i < 2; i++)
        {
            mRecommendedTexSize[i] = ovr_GetFovTextureSize(mSession, ovrEyeType(i), mHmdDesc.DefaultEyeFov[i], mPixelDensity);
        }

        mState.mIsVisible = true;
        mState.mShouldQuit = false;
        mState.mHMDLost = false;

        iInitValues();


        return true;
    }
    bool piVROculus::Init(const char * appID, int deviceID, float pixelDensity, piLog* log, piTimer* timer)
    {
        mLog = log;
        mPlatform.mUserAccessToken.Init(256);
        mPlatform.mAppVersion.Init(256);

        if (ovr_Initialize(nullptr) < 0)
            return false;

        if (appID != nullptr)
        {
			if (ovr_PlatformInitializeWindows(appID) != ovrPlatformInitialize_Success)
				return false;
            ovr_Entitlement_GetIsViewerEntitled(); // send message
            ovr_User_GetAccessToken();
            ovr_User_GetLoggedInUser();
			mPlatform.mUserLocale = ovr_GetLoggedInUserLocale();
			ovr_Application_GetVersion();
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"Starting OVR Platform, sent entitlement check and user request");
#endif // _DEBUG
        }

        mTimer = timer;

        ovrGraphicsLuid luid;
        ovrResult res = ovr_Create(&mSession, &luid);
        if (!OVR_SUCCESS(res))
            return false;

        res = ovr_GetAudioDeviceOutGuid(&mSoundGUID);
        res = ovr_GetAudioDeviceOutGuidStr(mSoundName);

        mHmdDesc = ovr_GetHmdDesc(mSession);
		if (mHmdDesc.Type <= ovrHmd_CV1)
			mType = piVRHMD::Oculus_Rift;
		else if (mHmdDesc.Type == ovrHmd_RiftS)
			mType = piVRHMD::Oculus_RiftS;
		else
			mType = piVRHMD::Oculus_Quest;

        mEnableMipmapping = (pixelDensity > 1.0f);
        mPixelDensity = pixelDensity;

        for (int i = 0; i < 2; i++)
        {
            mRecommendedTexSize[i] = ovr_GetFovTextureSize(mSession, ovrEyeType(i), mHmdDesc.DefaultEyeFov[i], pixelDensity);
        }


        iInitValues();
        return true;
    }

    void piVROculus::DeInit(void)
    {
        mPlatform.mUserAccessToken.End();
        mPlatform.mAppVersion.End();
        if (mUsingMirrorTexture) ovr_DestroyMirrorTexture(mSession, mMirrorTexture);
        ovr_Destroy(mSession);
        ovr_Shutdown();
    }

    void piVROculus::GetLastError(char* errorStr)
    {
        ovrErrorInfo errorInfo;
        ovr_GetLastErrorInfo(&errorInfo);
        pistrcpy(errorStr, 511, errorInfo.ErrorString);
    }

    bool piVROculus::AttachToWindow2(void *t1, void *t2)
    {
        return true;
    }

    bool piVROculus::AttachToWindow(bool createMirrorTexture, int mirrorTextureSizeX, int mirrorTextureSizeY)
    {
        mUsingMirrorTexture = false;
        if (createMirrorTexture)
        {
            mUsingMirrorTexture = true;
            ovrMirrorTextureDesc desc;
            memset(&desc, 0, sizeof(desc));
            desc.Width = mirrorTextureSizeX;
            desc.Height = mirrorTextureSizeY;
            desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
            desc.MiscFlags = 0;// ovrTextureMisc_AutoGenerateMips;
            desc.MirrorOptions = ovrMirrorOption_LeftEyeOnly | ovrMirrorOption_IncludeSystemGui;
            ovrResult result = ovr_CreateMirrorTextureWithOptionsGL(mSession, &desc, &mMirrorTexture);
            if (!OVR_SUCCESS(result))
                return false;
            ovr_GetMirrorTextureBufferGL(mSession, mMirrorTexture, &mInfo.mMirrorTexID);
        }

        for (int i = 0; i < 2; i++)
        {
            //------------
            {
                ovrTextureSwapChainDesc desc = {};
                desc.Type = ovrTexture_2D;
                desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
                desc.ArraySize = 1;
                desc.Width = mRecommendedTexSize[i].w;
                desc.Height = mRecommendedTexSize[i].h;
                desc.MipLevels = (mEnableMipmapping == true) ? 3 : 1;
                desc.SampleCount = 1;
                desc.StaticImage = ovrFalse;

                ovrResult result = ovr_CreateTextureSwapChainGL(mSession, &desc, &mTextureChainColor[i]);
                if (!OVR_SUCCESS(result))
                    return false;

                int length = 0;
                ovr_GetTextureSwapChainLength(mSession, mTextureChainColor[i], &length);

                mInfo.mTexture[i].mNum = length;

                for (int j = 0; j < length; j++)
                {
                    unsigned int chainTexId;
                    ovr_GetTextureSwapChainBufferGL(mSession, mTextureChainColor[i], j, &chainTexId);
                    mInfo.mTexture[i].mTexIDColor[j] = chainTexId;
                }
            }
#ifdef PROVIDE_DEPTH
            //------------
            {
                ovrTextureSwapChainDesc desc = {};
                desc.Type = ovrTexture_2D;
                desc.Format = OVR_FORMAT_D24_UNORM_S8_UINT; // OVR_FORMAT_D32_FLOAT
                desc.ArraySize = 1;
                desc.Width = mRecommendedTexSize[i].w;
                desc.Height = mRecommendedTexSize[i].h;
                desc.MipLevels = 1;
                desc.SampleCount = 1;
                desc.StaticImage = ovrFalse;

                ovrResult result = ovr_CreateTextureSwapChainGL(mSession, &desc, &mTextureChainDepth[i]);
                if (!OVR_SUCCESS(result))
                    return false;

                int length = 0;
                ovr_GetTextureSwapChainLength(mSession, mTextureChainDepth[i], &length);

                if (length != mInfo.mTexture[i].mNum)
                    return false;

                mInfo.mTexture[i].mNum = length;

                for (int j = 0; j < length; j++)
                {
                    unsigned int chainTexId;
                    ovr_GetTextureSwapChainBufferGL(mSession, mTextureChainDepth[i], j, &chainTexId);
                    mInfo.mTexture[i].mTexIDDepth[j] = chainTexId;
                }
            }
#endif
            //------------

            mLayer.ColorTexture[i] = mTextureChainColor[i];
#ifdef PROVIDE_DEPTH
            mLayer.DepthTexture[i] = mTextureChainDepth[i];
#endif
            mLayer.Viewport[i] = { { 0, 0 }, { mRecommendedTexSize[i].w, mRecommendedTexSize[i].h } };
        }


        // FloorLevel will give tracking poses where the floor height is 0
        ovr_SetTrackingOriginType(mSession, ovrTrackingOrigin_FloorLevel);
        //ovr_SetTrackingOriginType(mSession, ovrTrackingOrigin_EyeLevel);

         // Initialize our single full screen Fov layer.
#ifdef PROVIDE_DEPTH
        mLayer.Header.Type = ovrLayerType_EyeFovDepth;
#else
        mLayer.Header.Type = ovrLayerType_EyeFov;
#endif
        mLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft | ovrLayerFlag_HighQuality;
        return true;
    }

    void piVROculus::GetControllersAvailable(bool *resLeftTouch, bool *resRightTouch, bool *resRemote)
    {
        ovrInputState inputState;

        bool remote = false;
        bool leftTouch = false;
        bool rightTouch = false;

        if (ovr_GetInputState(mSession, ovrControllerType_Remote, &inputState) >= 0)
        {
            remote = (inputState.ControllerType & ovrControllerType_Remote) != 0;
        }
        if (ovr_GetInputState(mSession, ovrControllerType_Touch, &inputState) >= 0)
        {
            leftTouch = ((inputState.ControllerType & ovrControllerType_LTouch) != 0);
            rightTouch = ((inputState.ControllerType & ovrControllerType_RTouch) != 0);
        }

        *resLeftTouch = leftTouch;
        *resRightTouch = rightTouch;
        *resRemote = remote;
    }

    void piVROculus::BeginFrame(int *texIndexLeft, int *texIndexRight, bool *outNeedsMipMapping)
    {
        // update mFrame now, so it is correct in call to ovr_GetPredictedDisplayTime below
        mFrame++;

        // take care of haptics first
        for (int i = 0; i < 2; i++)
        {
            HapticState *hs = mHapticState + i;
            if (hs->mState == 1)
            {
                const float dt = float(mTimer->GetTime() - hs->mTime);
                if (dt > hs->mDuration)
                {
                    const ovrControllerType ct = (i == 0) ? ovrControllerType_LTouch : ovrControllerType_RTouch;
                    ovr_SetControllerVibration(mSession, ct, 0.0f, 0.0f);
                    hs->mState = 0;
                }
            }
        }

        // go on with rendering

        ovrEyeRenderDesc    eyeRenderDesc[2];

        eyeRenderDesc[0] = ovr_GetRenderDesc(mSession, ovrEye_Left, mHmdDesc.DefaultEyeFov[0]);
        eyeRenderDesc[1] = ovr_GetRenderDesc(mSession, ovrEye_Right, mHmdDesc.DefaultEyeFov[1]);

        ovrPosef HmdToEyeOffset[2] = { eyeRenderDesc[0].HmdToEyePose, eyeRenderDesc[1].HmdToEyePose };

        double   sensorSampleTime;
        ovrPosef EyeRenderPose[2];
        ovr_GetEyePoses(mSession, mFrame, ovrTrue, HmdToEyeOffset, EyeRenderPose, &sensorSampleTime);

        mLayer.Fov[0] = eyeRenderDesc[0].Fov;
        mLayer.Fov[1] = eyeRenderDesc[1].Fov;
        mLayer.RenderPose[0] = EyeRenderPose[0];
        mLayer.RenderPose[1] = EyeRenderPose[1];
        mLayer.SensorSampleTime = sensorSampleTime;

        // Get both eye poses simultaneously, with IPD offset already included.
        double predictedDisplayInSeconds = ovr_GetPredictedDisplayTime(mSession, mFrame);
        ovrTrackingState hmdState = ovr_GetTrackingState(mSession, predictedDisplayInSeconds, ovrTrue);
        ovr_CalcEyePoses(hmdState.HeadPose.ThePose, HmdToEyeOffset, mLayer.RenderPose);

        ovrTrackerPose trackerPose = ovr_GetTrackerPose(mSession, 0);
        mState.mTrackerConnected = (trackerPose.TrackerFlags & ovrTracker_Connected) != 0;
        mState.mPositionTracked = (hmdState.StatusFlags & ovrStatus_PositionTracked) != 0;

        mat4x4 rot = mat4x4::rotate(quat(&hmdState.HeadPose.ThePose.Orientation.x));
        mat4x4 tra = mat4x4::translate(-hmdState.HeadPose.ThePose.Position.x, -hmdState.HeadPose.ThePose.Position.y, -hmdState.HeadPose.ThePose.Position.z);
        mat4x4 tmp = transpose(rot) * tra;
        memcpy(mInfo.mHead.mCamera, &tmp, 16 * sizeof(float));

        const uint64_t time = mTimer->GetTimeMs();

        ovrInputState inputState;

        if (ovr_GetInputState(mSession, ovrControllerType_Remote, &inputState) >= 0)
        {
            mInfo.mRemote.mEnabled = (inputState.ControllerType & ovrControllerType_Remote) != 0;

            iButtonLogic(&mInfo.mRemote.mUpButton, (inputState.Buttons & ovrButton_Up) != 0, time);
            iButtonLogic(&mInfo.mRemote.mDownButton, (inputState.Buttons & ovrButton_Down) != 0, time);
            iButtonLogic(&mInfo.mRemote.mLeftButton, (inputState.Buttons & ovrButton_Left) != 0, time);
            iButtonLogic(&mInfo.mRemote.mRightButton, (inputState.Buttons & ovrButton_Right) != 0, time);

            iButtonLogic(&mInfo.mRemote.mEnterButton, (inputState.Buttons & ovrButton_Enter) != 0, time);
            iButtonLogic(&mInfo.mRemote.mVolumeUpButton, (inputState.Buttons & ovrButton_VolUp) != 0, time);
            iButtonLogic(&mInfo.mRemote.mVolumeUpButton, (inputState.Buttons & ovrButton_VolDown) != 0, time);
            iButtonLogic(&mInfo.mRemote.mHomeButton, (inputState.Buttons & ovrButton_Home) != 0, time);
        }

        if (ovr_GetInputState(mSession, ovrControllerType_Touch, &inputState) >= 0)
        {
            mInfo.mController[0].mEnabled = ((inputState.ControllerType & ovrControllerType_LTouch) != 0);
            if (mInfo.mController[0].mEnabled)
            {
                iJoystickLogic(&mInfo.mController[0].mThumbstick, inputState.Thumbstick[ovrHand_Left].x, inputState.Thumbstick[ovrHand_Left].y);

                mInfo.mController[0].mIndexTrigger = inputState.IndexTrigger[ovrHand_Left];
                mInfo.mController[0].mHandTrigger = inputState.HandTrigger[ovrHand_Left];

                iButtonLogic(&mInfo.mController[0].mThumbstickButton, (inputState.Buttons & ovrButton_LThumb) != 0, time);
                iButtonLogic(&mInfo.mController[0].mUpButton, (inputState.Buttons & ovrButton_Y) != 0, time);
                iButtonLogic(&mInfo.mController[0].mDownButton, (inputState.Buttons & ovrButton_X) != 0, time);
                iButtonLogic(&mInfo.mController[0].mMenuButton, (inputState.Buttons & ovrButton_Enter) != 0, time);

                iTouchLogic(&mInfo.mController[0].mThumbstickTouch, (inputState.Touches & ovrTouch_LThumb) != 0);
                iTouchLogic(&mInfo.mController[0].mDownTouch, (inputState.Touches & ovrTouch_X) != 0);
                iTouchLogic(&mInfo.mController[0].mUpTouch, (inputState.Touches & ovrTouch_Y) != 0);
                iTouchLogic(&mInfo.mController[0].mIndexTriggerTouch, (inputState.Touches & ovrTouch_LIndexTrigger) != 0);
            }

            mInfo.mController[1].mEnabled = ((inputState.ControllerType & ovrControllerType_RTouch) != 0);
            if (mInfo.mController[1].mEnabled)
            {
                mInfo.mController[1].mEnabled = true;

                iJoystickLogic(&mInfo.mController[1].mThumbstick, inputState.Thumbstick[ovrHand_Right].x, inputState.Thumbstick[ovrHand_Right].y);

                mInfo.mController[1].mIndexTrigger = inputState.IndexTrigger[ovrHand_Right];
                mInfo.mController[1].mHandTrigger = inputState.HandTrigger[ovrHand_Right];

                iButtonLogic(&mInfo.mController[1].mThumbstickButton, (inputState.Buttons & ovrButton_RThumb) != 0, time);
                iButtonLogic(&mInfo.mController[1].mUpButton, (inputState.Buttons & ovrButton_B) != 0, time);
                iButtonLogic(&mInfo.mController[1].mDownButton, (inputState.Buttons & ovrButton_A) != 0, time);
                iButtonLogic(&mInfo.mController[1].mMenuButton, (inputState.Buttons & ovrButton_Home) != 0, time);

                iTouchLogic(&mInfo.mController[1].mThumbstickTouch, (inputState.Touches & ovrTouch_RThumb) != 0);
                iTouchLogic(&mInfo.mController[1].mDownTouch, (inputState.Touches & ovrTouch_A) != 0);
                iTouchLogic(&mInfo.mController[1].mUpTouch, (inputState.Touches & ovrTouch_B) != 0);
                iTouchLogic(&mInfo.mController[1].mIndexTriggerTouch, (inputState.Touches & ovrTouch_RIndexTrigger) != 0);
            }
        }
        else
        {
            mInfo.mController[0].mEnabled = false;
            mInfo.mController[1].mEnabled = false;
        }

        for (int i = 0; i < 2; i++)
        {
            ovrPosef ph0 = hmdState.HandPoses[i].ThePose;

            ovrVector3f vel = hmdState.HandPoses[i].LinearVelocity;
            ovrVector3f angVel = hmdState.HandPoses[i].AngularVelocity;

            mInfo.mController[i].mPosition[0] = ph0.Position.x;
            mInfo.mController[i].mPosition[1] = ph0.Position.y;
            mInfo.mController[i].mPosition[2] = ph0.Position.z;
            mInfo.mController[i].mRotation[0] = ph0.Orientation.x;
            mInfo.mController[i].mRotation[1] = ph0.Orientation.y;
            mInfo.mController[i].mRotation[2] = ph0.Orientation.z;
            mInfo.mController[i].mRotation[3] = ph0.Orientation.w;

            mInfo.mController[i].mVelocity[0] = vel.x;
            mInfo.mController[i].mVelocity[1] = vel.y;
            mInfo.mController[i].mVelocity[2] = vel.z;
            mInfo.mController[i].mAngularVelocity[0] = angVel.x;
            mInfo.mController[i].mAngularVelocity[1] = angVel.y;
            mInfo.mController[i].mAngularVelocity[2] = angVel.z;

        }


        //ovrSwapTextureSet *ts = mTextureSet;

        //if (isVisible)
        {
            //ts->CurrentIndex = (ts->CurrentIndex + 1) % ts->TextureCount;

            for (int eyeID = 0; eyeID < 2; eyeID++)
            {
                mat4x4 rot = mat4x4::rotate(quat(&mLayer.RenderPose[eyeID].Orientation.x));
                mat4x4 tra = mat4x4::translate(-mLayer.RenderPose[eyeID].Position.x, -mLayer.RenderPose[eyeID].Position.y, -mLayer.RenderPose[eyeID].Position.z);
                mat4x4 tmp = transpose(rot) * tra;
                memcpy(mInfo.mEye[eyeID].mCamera, &tmp, 16 * sizeof(float));

                mInfo.mEye[eyeID].mProjection[0] = mLayer.Fov[eyeID].UpTan;
                mInfo.mEye[eyeID].mProjection[1] = mLayer.Fov[eyeID].DownTan;
                mInfo.mEye[eyeID].mProjection[2] = mLayer.Fov[eyeID].LeftTan;
                mInfo.mEye[eyeID].mProjection[3] = mLayer.Fov[eyeID].RightTan;

                mInfo.mEye[eyeID].mVP[0] = mLayer.Viewport[eyeID].Pos.x;
                mInfo.mEye[eyeID].mVP[1] = mLayer.Viewport[eyeID].Pos.y;
                mInfo.mEye[eyeID].mVP[2] = mLayer.Viewport[eyeID].Size.w;
                mInfo.mEye[eyeID].mVP[3] = mLayer.Viewport[eyeID].Size.h;
            }
        }


        mInfo.mHead.mProjection[0] = mLayer.Fov[0].UpTan;
        mInfo.mHead.mProjection[1] = mLayer.Fov[0].DownTan;
        mInfo.mHead.mProjection[2] = mLayer.Fov[0].LeftTan;
        mInfo.mHead.mProjection[3] = mLayer.Fov[1].RightTan;

        *outNeedsMipMapping = mEnableMipmapping;
        ovr_GetTextureSwapChainCurrentIndex(mSession, mTextureChainColor[0], texIndexLeft);
        ovr_GetTextureSwapChainCurrentIndex(mSession, mTextureChainColor[1], texIndexRight);
    }

    void piVROculus::EndFrame(void)
    {
        for (int i = 0; i < 2; i++)
        {
            ovr_CommitTextureSwapChain(mSession, mTextureChainColor[i]);
#ifdef PROVIDE_DEPTH
            ovr_CommitTextureSwapChain(mSession, mTextureChainDepth[i]);
#endif
        }

        // Submit frame with one layer we have.
        ovrLayerHeader* layers = &mLayer.Header;
        ovrResult       result = ovr_SubmitFrame(mSession, mFrame, nullptr, &layers, 1);
        ovrSessionStatus sessionStatus;

        if (result == ovrSuccess_NotVisible)
        {
            mState.mIsVisible = false;
        }
        else if (result == ovrError_DisplayLost)
        {
            mState.mShouldQuit = true;
            mState.mHMDLost = true;
            // We can either immediately quit or do the following:
            /*
            <destroy render target and graphics device>
                ovr_Destroy(session);

            do
            { // Spin while trying to recreate session.
                result = ovr_Create(&session, &luid);
            }
            while (OVR_FAILURE(result) && !shouldQuit);

            if (OVR_SUCCESS(result))
            {
                <recreate graphics device with luid>
                    <recreate render target via ovr_CreateTextureSwapChain>
            }
            */
        }
        else if (OVR_FAILURE(result))
        {
            mState.mShouldQuit = true;
        }
        else
        {
            mState.mIsVisible = true;
        }

        ovr_GetSessionStatus(mSession, &sessionStatus);

        if (sessionStatus.ShouldQuit)
        {
            mState.mShouldQuit = true;
        }

        if (sessionStatus.ShouldRecenter)
        {
            ovr_RecenterTrackingOrigin(mSession); // or ovr_ClearShouldRecenterFlag(session) to ignore the request.
        }

        iProcessOVRMessages();

        if (mNetwork.mNumPeers > 0)
        {
            iProcessNetPackets();
        }
    }

    void piVROculus::SetTrackingOrigin(void)
    {
        ovr_RecenterTrackingOrigin(mSession);
    }

    void piVROculus::GetHmdState(HmdState* state)
    {
        ovrSessionStatus sessionStatus;
        ovr_GetSessionStatus(mSession, &sessionStatus);
        // these can also be set to true in endframe, so we only set them here if true
        if (sessionStatus.IsVisible) mState.mIsVisible = true;
        if (sessionStatus.ShouldQuit) mState.mShouldQuit = true;
        state->mIsVisible = mState.mIsVisible;
        state->mHMDWorn = (sessionStatus.HmdMounted != 0);
        state->mShouldQuit = mState.mShouldQuit;
        state->mHMDLost = mState.mHMDLost;
        state->mTrackerConnected = mState.mTrackerConnected;
        state->mPositionTracked = mState.mPositionTracked;
    }


    void * piVROculus::GetSoundOutputGUID(void)
    {
        return (void*)&mSoundGUID;
    }

    wchar_t *piVROculus::GetSoundOutputName(void)
    {
        return mSoundName;
    }

    void piVROculus::SetTrackingOriginType(TrackingOrigin type)
    {
        switch (type)
        {
        case FloorLevel: ovr_SetTrackingOriginType(mSession, ovrTrackingOrigin_FloorLevel); break;
        case EyeLevel: ovr_SetTrackingOriginType(mSession, ovrTrackingOrigin_EyeLevel); break;
        }
    }

    bool piVROculus::LogAllEvents(std::string eventName, std::vector<std::map<std::string, std::string>> eventBatch)
    {
        // starting to submitting data
        for (int i = 0; i < eventBatch.size(); i++)
        {
            std::map<std::string, std::string> event = eventBatch[i];
            std::vector<ovrPlatformLogValue> logData;
            for (auto subIter = event.begin(); subIter != event.end(); subIter++)
            {
                ovrPlatformLogValue data{ subIter->first.c_str(), subIter->second.c_str() };
                logData.push_back(data);
            }
            ovr_Log_NewEvent(eventName.c_str(), logData.data(), logData.size());
        }
        return true;
    }

    void piVROculus::CreateAndJoinPrivateRoom(void)
    {
        ovr_Room_CreateAndJoinPrivate2(ovrRoom_JoinPolicyEveryone, 4, nullptr);
    }

    void piVROculus::LaunchInviteUsersFlow(void)
    {
        ovr_Room_LaunchInvitableUserFlow(mNetwork.mRoomID);
    }

    void piVROculus::SendPacketToUser(uint64_t userID, const void * data, size_t length, bool useTCP)
    {
        ovr_Net_SendPacket(userID, length, data, useTCP ? ovrSend_Reliable : ovrSend_Unreliable);
    }

    void piVROculus::SendPacketToRoom(const void * data, size_t length, bool useTCP)
    {
        ovr_Net_SendPacketToCurrentRoom(length, data, useTCP ? ovrSend_Reliable : ovrSend_Unreliable);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void piVROculus::iProcessOVRMessages()
    {
        ovrMessage* message = nullptr;

        while ((message = ovr_PopMessage()) != nullptr)
        {
            switch (ovr_Message_GetType(message))
            {
            case ovrMessage_Room_CreateAndJoinPrivate2:       iProcessCreateRoomResponse(message); break;
            case ovrMessage_Room_Join:
            case ovrMessage_Room_Join2:                       iProcessJoinRoomResponse(message); break;
            case ovrMessage_Room_Leave:                       iProcessLeaveRoomResponse(message); break;
            case ovrMessage_Notification_Room_RoomUpdate:     iProcessUpdateRoom(message); break;
            case ovrMessage_Notification_Room_InviteAccepted: iProcessInviteAccepted(message); break;
            case ovrMessage_Notification_Room_InviteReceived: iProcessInviteReceived(message); break;
            case ovrMessage_Notification_GetRoomInvites:      iProcessInvitesReceived(message); break;

            case ovrMessage_User_GetAccessToken:              iProcessAccessToken(message); break;
            case ovrMessage_User_GetLoggedInUser:             iProcessLoggedIn(message); break;
            case ovrMessage_Entitlement_GetIsViewerEntitled:  iProcessEntitlement(message); break;

            case ovrMessage_Notification_Voip_ConnectRequest: iProcessVoipPeerConnect(message); break;
            case ovrMessage_Notification_Voip_StateChange:    iProcessVoipStateChange(message); break;

            case ovrMessage_Notification_Networking_PeerConnectRequest:    iProcessNetworkingPeerConnect(message); break;
            case ovrMessage_Notification_Networking_ConnectionStateChange: iProcessNetworkingStateChange(message); break;

            case ovrMessage_Notification_ApplicationLifecycle_LaunchIntentChanged: iProcessLaunchIntentChanged(message); break;

			case ovrMessage_Application_GetVersion: iProcessApplicationVersion(message); break;
            }
            ovr_FreeMessage(message);
        }
    }

    void piVROculus::iHaptic(int id, float frequency, float amplitude, float duration)
    {
        HapticState *hs = mHapticState + id;

        if (hs->mState == 1) return;

        hs->mState = 1;
        hs->mDuration = duration;
        hs->mTime = mTimer->GetTime();

        const ovrControllerType ct = (id == 0) ? ovrControllerType_LTouch : ovrControllerType_RTouch;
        ovr_SetControllerVibration(mSession, ct, frequency, amplitude);
    }

    void piVROculus::iProcessInviteReceived(ovrMessage * message)
    {
        ovrRoomInviteNotificationHandle invite;
        invite = ovr_Message_GetRoomInviteNotification(message);
        ovrID roomID = ovr_RoomInviteNotification_GetRoomID(invite);
#ifdef _DEBUG
        mLog->Printf(LT_DEBUG, L"Received invitation to join %llu", roomID);
#endif // _DEBUG
        ovr_Room_Join2(roomID, nullptr);
    }

    void piVROculus::iProcessInvitesReceived(ovrMessage * message)
    {
        ovrRoomInviteNotificationArrayHandle invites;
        invites = ovr_Message_GetRoomInviteNotificationArray(message);

        if (ovr_RoomInviteNotificationArray_GetSize(invites) > 0)
        {
            ovrRoomInviteNotificationHandle invite = ovr_RoomInviteNotificationArray_GetElement(invites, 0);
            ovrID roomID = ovr_RoomInviteNotification_GetRoomID(invite);
            mLog->Printf(LT_DEBUG, L"Received invitation to join %llu, joining...", roomID);
            ovr_Room_Join2(roomID, nullptr);
        }
    }

    void piVROculus::iProcessInviteAccepted(ovrMessage * message)
    {
        const char *roomIDString = ovr_Message_GetString(message);
        ovrID roomID;
        ovrID_FromString(&roomID, roomIDString);
        mLog->Printf(LT_DEBUG, L"Accepted invitation to join %llu, joining...", roomID);
        ovr_Room_Join2(roomID, nullptr);
    }

    void piVROculus::iProcessLoggedIn(ovrMessage * message)
    {
        if (!ovr_Message_IsError(message))
        {
            ovrUser* myUser = ovr_Message_GetUser(message);
            mPlatform.mUserID = ovr_User_GetID(myUser);
            mPlatform.mLoggedIn = true;
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"OVR Logged in userID %llu", mPlatform.mUserID);
#endif // _DEBUG
            ovr_Notification_GetRoomInvites();
        }
        else
        {
            const ovrErrorHandle error = ovr_Message_GetError(message);
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"Received get current logged in user failure: %s, retrying...", ovr_Error_GetMessage(error));
#endif // _DEBUG
            mPlatform.mLoggedIn = false;
            // We failed to get the current logged in user. Retry.

			ovrRequest req;
			req = ovr_User_GetLoggedInUser();

        }
    }

    void piVROculus::iProcessAccessToken(ovrMessage * message)
    {
        if (!ovr_Message_IsError(message))
        {
			mPlatform.mUserAccessToken.CopyS(ovr_Message_GetString(message));
#ifdef _DEBUG
			mLog->Printf(LT_DEBUG, L"OVR current user access token: %s", mPlatform.mUserAccessToken.GetS());
#endif // _DEBUG
        }
		else
		{
			const ovrErrorHandle error = ovr_Message_GetError(message);
#ifdef _DEBUG
			mLog->Printf(LT_DEBUG, L"Received get current user access token failure, retrying...");
#endif // _DEBUG

			// We failed to get the current user access token. Retry.
			ovrRequest req;
			req = ovr_User_GetAccessToken();

		}
    }

    void piVROculus::iProcessNetPackets()
    {
        ovrPacketHandle packet;
        while (packet = ovr_Net_ReadPacket())
        {
            ovrID senderID = ovr_Packet_GetSenderID(packet);

            if (mNetwork.mOnPeerPacket)
                mNetwork.mOnPeerPacket(senderID, ovr_Packet_GetBytes(packet), ovr_Packet_GetSize(packet));

            ovr_Packet_Free(packet);
        }
    }

    void piVROculus::iProcessVoipPeerConnect(ovrMessage* message)
    {
        if (!ovr_Message_IsError(message))
        {
            ovrNetworkingPeerHandle netPeer = ovr_Message_GetNetworkingPeer(message);
            ovr_Voip_Accept(ovr_NetworkingPeer_GetID(netPeer));
            mLog->Printf(LT_DEBUG, L"Received Voip connect request, accepted.\n");
        }
        else
        {
            const ovrErrorHandle error = ovr_Message_GetError(message);
            mLog->Printf(LT_DEBUG, L"Received Voip connect failure: %s\n", ovr_Error_GetMessage(error));
        }
    }

    void piVROculus::iProcessVoipStateChange(ovrMessage* message)
    {
        if (!ovr_Message_IsError(message))
        {
            ovrNetworkingPeer* netPeer = ovr_Message_GetNetworkingPeer(message);
            const ovrID peerID = ovr_NetworkingPeer_GetID(netPeer);

            switch (ovr_NetworkingPeer_GetState(netPeer))
            {
            case ovrPeerState_Connected: mLog->Printf(LT_DEBUG, L"Received voip state change Connected from: %llu", peerID); iStartVoipSoundBuffer(peerID); break;
            case ovrPeerState_Timeout:mLog->Printf(LT_DEBUG, L"Received voip state change Timeout from: %llu", peerID); break; // to do reconnect
            case ovrPeerState_Closed: mLog->Printf(LT_DEBUG, L"Received voip state change Closed from: %llu", peerID); iStopVoipSoundBuffer(peerID); break;
            case ovrPeerState_Unknown:
            default:
                break;
            }
        }
        else
        {
            const ovrErrorHandle error = ovr_Message_GetError(message);
            mLog->Printf(LT_DEBUG, L"Received voip state change failure: %s\n", ovr_Error_GetMessage(error));
        }
    }
    void piVROculus::iProcessEntitlement(ovrMessage * message)
    {
        if (!ovr_Message_IsError(message))
        {
            mPlatform.mNotEntitled = false;
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"User is entitled");
#endif // _DEBUG
        }
        else
        {

#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"User not entitled");
#endif // _DEBUG
            mPlatform.mNotEntitled = true;
        }
    }

    void piVROculus::iProcessLaunchIntentChanged(ovrMessage * message)
    {
		if (!ovr_Message_IsError(message))
		{
			ovrLaunchDetailsHandle ld = ovr_ApplicationLifecycle_GetLaunchDetails();
			mPlatform.mDeepLinkMessage = ovr_LaunchDetails_GetDeeplinkMessage(ld);
		}
    }

	void piVROculus::iProcessApplicationVersion(ovrMessage * message)
    {
		if (!ovr_Message_IsError(message))
		{
			ovrApplicationVersionHandle app = ovr_Message_GetApplicationVersion(message);
			mPlatform.mAppVersion.CopyS(ovr_ApplicationVersion_GetCurrentName(app));
		}

    }

    void piVROculus::iProcessCreateRoomResponse(ovrMessage * message)
    {
        if (!ovr_Message_IsError(message))
        {
            ovrRoom* newRoom = ovr_Message_GetRoom(message);
            mNetwork.mRoomID = ovr_Room_GetID(newRoom);
            mNetwork.mNumPeers = 0;
            mLog->Printf(LT_DEBUG, L"Room create success, roomID %llu", mNetwork.mRoomID);
        }
        else
        {
            mLog->Printf(LT_DEBUG, L"Room create failed.");
            mNetwork.mRoomID = 0;
        }
    }

    void piVROculus::iProcessJoinRoomResponse(ovrMessage * message)
    {
        if (!ovr_Message_IsError(message))
        {
            // Try to pull out remote user's ID if they have already joined
            ovrRoom* newRoom = ovr_Message_GetRoom(message);
            mNetwork.mRoomID = ovr_Room_GetID(newRoom);

            mLog->Printf(LT_DEBUG, L"Received join room success room %llu", mNetwork.mRoomID);

            mNetwork.mNumPeers = 0;
            ovrUserArray* users = ovr_Room_GetUsers(newRoom);
            for (size_t x = 0; x < ovr_UserArray_GetSize(users); x++)
            {
                ovrUser* nextUser = ovr_UserArray_GetElement(users, x);
                if (mPlatform.mUserID != ovr_User_GetID(nextUser))
                {
                    mNetwork.mPeers[mNetwork.mNumPeers].mUserID = ovr_User_GetID(nextUser);
                    ovr_Voip_Start(mNetwork.mPeers[mNetwork.mNumPeers].mUserID);

                    if (mNetwork.mOnUserConnect)
                        mNetwork.mOnUserConnect(mNetwork.mPeers[mNetwork.mNumPeers].mUserID);

                    mLog->Printf(LT_DEBUG, L"JoinRoom Adding peer %llu, start voip", mNetwork.mPeers[mNetwork.mNumPeers].mUserID);
                    mNetwork.mNumPeers++;
                }
            }
        }
        else
        {
            const ovrErrorHandle error = ovr_Message_GetError(message);
            mLog->Printf(LT_DEBUG, L"Received join room failure: %s", ovr_Error_GetMessage(error));
            mNetwork.mRoomID = 0;
        }
    }

    void piVROculus::iStartVoipSoundBuffer(uint64_t userID)
    {
        for (int i = 0; i < MAX_NETWORKING_PEERS; i++)
        {
            if (mNetwork.mPeers[i].mUserID == userID)
            {
                mNetwork.mPeers[i].mSoundID = mSoundEngine->AddSound([](float *buffer, size_t numSamples, size_t numChannels, void *userData)
                {
                    Peer * peer = (Peer*)userData;
                    ovr_Voip_GetPCMFloat(peer->mUserID, buffer, numSamples); //32 bit float 48k mono
                }, &mNetwork.mPeers[i]);

                mLog->Printf(LT_DEBUG, L"Starting soundID %d for user %llu", mNetwork.mPeers[i].mSoundID, userID);
                break;
            }
        }
    }

    void piVROculus::iStopVoipSoundBuffer(uint64_t userID)
    {
        for (int i = 0; i < MAX_NETWORKING_PEERS; i++)
        {
            if (mNetwork.mPeers[i].mUserID == userID)
            {
                mLog->Printf(LT_DEBUG, L"Stopping and deleting soundID %d for user %llu", mNetwork.mPeers[i].mSoundID, userID);
                mSoundEngine->Stop(mNetwork.mPeers[i].mSoundID);
                mSoundEngine->DelSound(mNetwork.mPeers[i].mSoundID);
                break;
            }
        }
    }

    void piVROculus::iProcessLeaveRoomResponse(ovrMessage * message)
    {
        if (!ovr_Message_IsError(message))
        {
            mLog->Printf(LT_DEBUG,L"We were able to leave room %llu", mNetwork.mRoomID);
            mNetwork.mRoomID = 0;
            mNetwork.mNumPeers = 0;
        }
        else
        {
            const ovrErrorHandle error = ovr_Message_GetError(message);
            mLog->Printf(LT_DEBUG,L"Received leave room failure: %s", ovr_Error_GetMessage(error));
        }
    }
    void piVROculus::iProcessUpdateRoom(ovrMessage * message)
    {
        if (!ovr_Message_IsError(message))
        {
            mLog->Printf(LT_DEBUG,L"Received room update notification");

            ovrRoom* updatedRoom = ovr_Message_GetRoom(message);
            ovrUserArray* users = ovr_Room_GetUsers(updatedRoom);

            if (ovr_UserArray_GetSize(users) < mNetwork.mNumPeers + 1)
            {
                // some users removed
                for (size_t i = 0; i < mNetwork.mNumPeers; i++)
                {
                    bool found = false;
                    for (size_t j = 0; j < ovr_UserArray_GetSize(users); j++)
                    {
                        ovrUser* nextUser = ovr_UserArray_GetElement(users, j);
                        const ovrID userID = ovr_User_GetID(nextUser);
                        if (mNetwork.mPeers[i].mUserID == userID)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found) // this user left
                    {
                        ovr_Voip_Stop(mNetwork.mPeers[i].mUserID);
                        iStopVoipSoundBuffer(mNetwork.mPeers[i].mUserID);
                        if (mNetwork.mOnUserLeave)
                            mNetwork.mOnUserLeave(mNetwork.mPeers[mNetwork.mNumPeers].mUserID);
                        mLog->Printf(LT_DEBUG, L"UpdateRoom Removing peer %llu, stop voip", mNetwork.mPeers[i].mUserID);
                        // remove and shift
                        for (size_t j = i; j < mNetwork.mNumPeers-1; j++)
                            mNetwork.mPeers[j] = mNetwork.mPeers[j + 1];
                        mNetwork.mNumPeers--;
                        i--;
                    }
                }
            }
            // check if users added
            for (size_t x = 0; x < ovr_UserArray_GetSize(users); x++)
            {
                ovrUser* nextUser = ovr_UserArray_GetElement(users, x);
                const ovrID userID = ovr_User_GetID(nextUser);
                if (mPlatform.mUserID != userID)
                {
                    bool found = false;
                    for (size_t y = 0; y < mNetwork.mNumPeers; y++)
                    {
                        if (mNetwork.mPeers[y].mUserID == userID)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found) // new user joined
                    {
                        mNetwork.mPeers[mNetwork.mNumPeers].mUserID = userID;
                        mLog->Printf(LT_DEBUG, L"UpdateRoom Adding peer %llu, start voip", mNetwork.mPeers[mNetwork.mNumPeers].mUserID);
                        ovr_Voip_Start(mNetwork.mPeers[mNetwork.mNumPeers].mUserID);
                        if (mNetwork.mOnUserConnect)
                            mNetwork.mOnUserConnect(mNetwork.mPeers[mNetwork.mNumPeers].mUserID);
                        mNetwork.mNumPeers++;
                    }
                }
            }
        }
        else
        {
            const ovrErrorHandle error = ovr_Message_GetError(message);
            mLog->Printf(LT_DEBUG,L"Received room Update failure: %s\n", ovr_Error_GetMessage(error));
        }
    }

    void piVROculus::iProcessNetworkingPeerConnect(ovrMessage * message)
    {
        if (!ovr_Message_IsError(message))
        {
            ovrNetworkingPeer* netPeer = ovr_Message_GetNetworkingPeer(message);
            ovrID peerID = ovr_NetworkingPeer_GetID(netPeer);
            mLog->Printf(LT_DEBUG,L"Received peer connect request success from %llu", ovr_NetworkingPeer_GetID(netPeer));
            ovr_Net_Accept(peerID);
        }
        else
        {
            const ovrErrorHandle error = ovr_Message_GetError(message);
            mLog->Printf(LT_DEBUG,L"Received peer connect request failure: %s", ovr_Error_GetMessage(error));
        }
    }

    void piVROculus::iProcessNetworkingStateChange(ovrMessage * message)
    {
        if (!ovr_Message_IsError(message))
        {
            ovrNetworkingPeer* netPeer = ovr_Message_GetNetworkingPeer(message);
            ovrID peerID = ovr_NetworkingPeer_GetID(netPeer);
            switch (ovr_NetworkingPeer_GetState(netPeer)) {
            case ovrPeerState_Connected: mLog->Printf(LT_DEBUG, L"Received networking state change Connected from: %llu", peerID); break;
            case ovrPeerState_Timeout:   mLog->Printf(LT_DEBUG, L"Received networking state change Timeout from: %llu", peerID); break;
            case ovrPeerState_Unknown:
            default:                     mLog->Printf(LT_DEBUG, L"Received networking state change Unknown from: %llu", peerID); break;
            }
        }
        else
        {
            const ovrErrorHandle error = ovr_Message_GetError(message);
            mLog->Printf(LT_DEBUG,L"Received networking state change failure: %s", ovr_Error_GetMessage(error));
        }
    }
}
