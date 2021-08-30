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

//#include "OVR_Platform_Internal.h"
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
        //mPlatform.mUserAccessToken.Init(256);
        //mPlatform.mAppVersion.Init(256);

        if (ovr_Initialize(nullptr) < 0)
            return false;
/*
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
*/
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
        //mPlatform.mUserAccessToken.End();
        //mPlatform.mAppVersion.End();
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
            //------------

            mLayer.ColorTexture[i] = mTextureChainColor[i];
            mLayer.Viewport[i] = { { 0, 0 }, { mRecommendedTexSize[i].w, mRecommendedTexSize[i].h } };
        }


        // FloorLevel will give tracking poses where the floor height is 0
        ovr_SetTrackingOriginType(mSession, ovrTrackingOrigin_FloorLevel);

         // Initialize our single full screen Fov layer.
        mLayer.Header.Type = ovrLayerType_EyeFov;
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

        {
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
            ovr_RecenterTrackingOrigin(mSession);
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
}
