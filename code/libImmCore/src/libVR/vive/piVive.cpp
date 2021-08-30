//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#if 0
#include "../../libBasics/piTimer.h"
#include "../../libBasics/piVecTypes.h"



#include "piVive.h"

namespace ImmCore
{

	//------------------------------------------------------------

	static void iTouchInit(piVRHMD::Touch *b)
	{
		b->mStateTouched = false;
		b->mEventTouchBegin = false;
		b->mEventTouchEnd = false;
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

	//------------------------------------------------------------

	piVRVive::piVRVive()
	{
		memset(&mData, 0, sizeof(piVRHMD::HmdInfo));
		mHMD = nullptr;
	}

	piVRVive::~piVRVive()
	{
	}

	bool piVRVive::Init(const char * appID, int deviceID, float pixelDensity)
	{
		if (!vr::VR_IsHmdPresent())
			return false;
		mType = piVRHMD::HTC_Vive;
		vr::EVRInitError eError = vr::VRInitError_None;
		mHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
		if (eError != vr::VRInitError_None)
		{
			return false;
		}

		if (!vr::VRCompositor())
		{
			return false;
		}

		if (!mTimer.Init())
			return false;

		mEnableMipmapping = (pixelDensity > 1.0f);

		uint32_t m_nRenderWidth;
		uint32_t m_nRenderHeight;
		mHMD->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);
		mInfo.mVRXres = m_nRenderWidth;
		mInfo.mVRYres = m_nRenderHeight;

		//mInfo.mXres = 0;
		//mInfo.mYres = 0;
		mInfo.mMirrorTexID = 0;

		mInfo.mTexture[0].mNum = 1;
		mInfo.mTexture[0].mTexIDColor[0] = 0;
		mInfo.mTexture[1].mNum = 1;
		mInfo.mTexture[1].mTexIDColor[0] = 0;


		for (int i = 0; i < 2; i++)
		{
			iButtonInit(&mInfo.mController[i].mUpButton);
			iButtonInit(&mInfo.mController[i].mDownButton);

			iTouchInit(&mInfo.mController[i].mUpTouch);
			iTouchInit(&mInfo.mController[i].mDownTouch);
			iTouchInit(&mInfo.mController[i].mIndexTriggerTouch);
			iTouchInit(&mInfo.mController[i].mThumbstickTouch);

			iJoystickInit(&mInfo.mController[i].mThumbstick);

			mInfo.mController[i].Vibrate = [&](int id, float frequency, float amplitude, float duration)
			{
				//iHaptic(id, frequency, amplitude, duration);
			};
			//mHapticState[i].mState = 0;
		}

		iButtonInit(&mInfo.mRemote.mDownButton);
		iButtonInit(&mInfo.mRemote.mUpButton);
		iButtonInit(&mInfo.mRemote.mLeftButton);
		iButtonInit(&mInfo.mRemote.mRightButton);
		iButtonInit(&mInfo.mRemote.mVolumeUpButton);
		iButtonInit(&mInfo.mRemote.mVolumeDownButton);
		iButtonInit(&mInfo.mRemote.mHomeButton);

		return true;
	}

	void piVRVive::DeInit(void)
	{
		vr::VR_Shutdown();
	}

	static int imax(int a, int b) { return (a > b) ? a : b; }


	void piVRVive::GetLastError(char * errorStr)
	{
	}

    void piVRVive::GetControllersAvailable(bool *leftTouch, bool *rightTouch, bool *remote)
    {
        *leftTouch = true;
        *rightTouch = true;
        *remote = true;
    }

	bool piVRVive::AttachToWindow(bool createMirrorTexture, int mirrorTextureSizeX, int mirrorTextureSizeY)
	{
		memset(mInfo.mHead.mCamera, 0, 16 * sizeof(float));
		for (int i = 0; i < 2; i++)
		{
			mInfo.mEye[0].mProjection[0] = 1.0f;
			mInfo.mEye[0].mProjection[1] = 1.0f;
			mInfo.mEye[0].mProjection[2] = 1.0f;
			mInfo.mEye[0].mProjection[3] = 1.0f;
			memset(mInfo.mEye[0].mCamera, 0, 16 * sizeof(float));
			mInfo.mEye[0].mVP[0] = 0;
			mInfo.mEye[0].mVP[1] = 0;
			mInfo.mEye[0].mVP[2] = mInfo.mVRXres;
			mInfo.mEye[0].mVP[3] = mInfo.mVRYres;
		}
		return true;
	}
	bool piVRVive::AttachToWindow2(void *t1, void *t2)
	{
		mInfo.mTexture[0].mTexIDColor[0] = (unsigned int)t1;
		mInfo.mTexture[1].mTexIDColor[0] = (unsigned int)t2;
		return true;
	}


	static mat4x4 vive2pilibs(const vr::HmdMatrix34_t & m)
	{
		return mat4x4(m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3],
			m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3],
			m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3],
			0.0f, 0.0f, 0.0f, 1.0f);
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

	static void iTouchLogic(piVRHMD::Touch *b, bool state)
	{
		bool oldState = b->mStateTouched;
		b->mStateTouched = state;
		b->mEventTouchBegin = (!oldState &&  b->mStateTouched);
		b->mEventTouchEnd = (oldState && !b->mStateTouched);
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

	void piVRVive::BeginFrame(int *texIndexLeft, int *texIndexRight, bool *outNeedsMipMapping)
	{
		*texIndexLeft = 0;
		*texIndexRight = 0;
		*outNeedsMipMapping = mEnableMipmapping;

		vr::VREvent_t event;
		while (mHMD->PollNextEvent(&event, sizeof(event)))
		{
			switch (event.eventType)
			{
			case vr::VREvent_TrackedDeviceDeactivated:
			{
				//dprintf("Device %u detached.\n", event.trackedDeviceIndex);
			}
			break;
			case vr::VREvent_TrackedDeviceUpdated:
			{
				//dprintf("Device %u updated.\n", event.trackedDeviceIndex);
			}
			break;
			}
		}

		vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::VRCompositor()->GetLastPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

		/*
		When you want non - blocking poses using the latest information, add calls to IVRSystem::GetDeviceToAbsoluteTrackingPose.Note that here you are asking the IVRSystem for a pose.
		When you want prediction, then feed IVRSystem::GetDeviceToAbsoluteTrackingPose with a future value
		When you want poses synced up with button presses, then use IVRSystem::PollNextEventWithPose
		When you want poses synced with controller state then use IVRSystem::GetControllerStateWithPose
		When you want poses synced with the compositor AND non - blocking then use IVRCompositor::GetLastPoses.Here you are asking the IVRCompositor for a pose.
		*/

		const mat4x4 head = invert(vive2pilibs(m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking));

		for (int eyeID = 0; eyeID < 2; eyeID++)
		{
			const vr::EVREye eye = (eyeID == 0) ? vr::Eye_Left : vr::Eye_Right;

			float fLeft, fRight, fTop, fBottom;
			mHMD->GetProjectionRaw(eye, &fLeft, &fRight, &fTop, &fBottom);

			const mat4x4 mat = invert(vive2pilibs(mHMD->GetEyeToHeadTransform(eye))) * head;

			memcpy(mInfo.mEye[eyeID].mCamera, &mat, 16 * sizeof(float));

			mInfo.mEye[eyeID].mProjection[0] = fabsf(fTop);
			mInfo.mEye[eyeID].mProjection[1] = fabsf(fBottom);
			mInfo.mEye[eyeID].mProjection[2] = fabsf(fLeft);
			mInfo.mEye[eyeID].mProjection[3] = fabsf(fRight);

			mInfo.mEye[eyeID].mVP[0] = 0;
			mInfo.mEye[eyeID].mVP[1] = 0;
			mInfo.mEye[eyeID].mVP[2] = mInfo.mVRXres;
			mInfo.mEye[eyeID].mVP[3] = mInfo.mVRYres;
		}

		memcpy(mInfo.mHead.mCamera, &head, 16 * sizeof(float));
		mInfo.mHead.mProjection[0] = mInfo.mEye[0].mProjection[0];
		mInfo.mHead.mProjection[1] = mInfo.mEye[0].mProjection[1];
		mInfo.mHead.mProjection[2] = mInfo.mEye[0].mProjection[2];
		mInfo.mHead.mProjection[3] = mInfo.mEye[1].mProjection[3]; // that 1 there is NOT a bug

		mInfo.mRemote.mEnabled = false;
		mInfo.mRemote.mEnterButton.mEventDown = false;

		const uint64_t time = mTimer.GetTimeMs();

		for (int i = 0; i < 2; i++)
		{

			const vr::ETrackedControllerRole role = (i == 0) ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand;
			const vr::TrackedDeviceIndex_t id = mHMD->GetTrackedDeviceIndexForControllerRole(role);

			mInfo.mController[i].mEnabled = mHMD->IsTrackedDeviceConnected(id);
			if (!mInfo.mController[i].mEnabled) continue;

			vr::VRControllerState_t controllerState;
			if (!mHMD->GetControllerState(id, &controllerState, sizeof(controllerState)))
			{
				mInfo.mController[i].mEnabled = false;
				continue;
			}

			//controllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Grip);
			//controllerState.ulButtonTouched;

			for (int j = 0; j < vr::k_unControllerStateAxisCount; j++)
			{
				const vr::ETrackedDeviceProperty prop = (vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32 + j);
				const vr::EVRControllerAxisType type = (vr::EVRControllerAxisType)mHMD->GetInt32TrackedDeviceProperty(id, prop);
				if (type == vr::k_eControllerAxis_TrackPad)
				{
					iJoystickLogic(&mInfo.mController[i].mThumbstick, controllerState.rAxis[j].x, controllerState.rAxis[j].y);
				}
				else if (type == vr::k_eControllerAxis_Trigger)
				{
					mInfo.mController[i].mIndexTrigger = controllerState.rAxis[j].x;
				}
				else if (type == vr::k_eControllerAxis_Joystick)
				{
					mInfo.mController[i].mHandTrigger = controllerState.rAxis[j].x;
				}

			}


			//iJoystickLogic(&mInfo.mController[i].mThumbstick, controllerState.rAxis[vr::k_eControllerAxis_TrackPad].x, controllerState.rAxis[vr::k_eControllerAxis_TrackPad].y);

			//iJoystickLogic(&mInfo.mController[i].mThumbstick, controllerState.rAxis[vr::k_eControllerAxis_Joystick].x, controllerState.rAxis[vr::k_eControllerAxis_Joystick].y);
			//k_unControllerStateAxisCount

			iButtonLogic(&mInfo.mController[i].mUpButton, (controllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu)) != 0, time);
			iButtonLogic(&mInfo.mController[i].mDownButton, (controllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Grip)) != 0, time);

			//iTouchLogic(&mInfo.mController[i].mThumbstickTouch, (inputState.Touches & ovrTouch_LThumb) != 0);
			//iTouchLogic(&mInfo.mController[i].mDownTouch, (inputState.Touches & ovrTouch_X) != 0);
			//iTouchLogic(&mInfo.mController[i].mUpTouch, (inputState.Touches & ovrTouch_Y) != 0);
			//iTouchLogic(&mInfo.mController[i].mIndexTriggerTouch, (inputState.Touches & ovrTouch_LIndexTrigger) != 0);

			const mat4x4 location = vive2pilibs(m_rTrackedDevicePose[id].mDeviceToAbsoluteTracking);

			memcpy(mInfo.mController[i].mLocation, &location, 16 * sizeof(float));
			mInfo.mController[i].mVelocity[0] = m_rTrackedDevicePose[id].vVelocity.v[0];
			mInfo.mController[i].mVelocity[1] = m_rTrackedDevicePose[id].vVelocity.v[1];
			mInfo.mController[i].mVelocity[2] = m_rTrackedDevicePose[id].vVelocity.v[2];
			mInfo.mController[i].mAngularVelocity[0] = m_rTrackedDevicePose[id].vAngularVelocity.v[0];
			mInfo.mController[i].mAngularVelocity[1] = m_rTrackedDevicePose[id].vAngularVelocity.v[1];
			mInfo.mController[i].mAngularVelocity[2] = m_rTrackedDevicePose[id].vAngularVelocity.v[2];

		}
	}

	void piVRVive::EndFrame(void)
	{
		vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)mInfo.mTexture[0].mTexIDColor[0], vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);

		vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)mInfo.mTexture[1].mTexIDColor[0], vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);

		vr::VRCompositor()->PostPresentHandoff();

		vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

	}

	void piVRVive::GetHmdState(HmdState * state)
	{
		state->mIsVisible = true;
		state->mHMDWorn = true;
		state->mShouldQuit = false;
		state->mHMDLost = false;
		state->mNotEntitled = false;
		state->mTrackerConnected = true;
		state->mPositionTracked = true;
	}

	void *piVRVive::GetSoundOutputGUID(void)
	{
		return nullptr;
	}

	void piVRVive::SetTrackingOrigin(void)
	{
	}

	void piVRVive::SetTrackingOriginType(TrackingOrigin type)
	{
	}

	bool piVRVive::RecreateHeadset(void)
	{
		return true;
	}

}
#endif
