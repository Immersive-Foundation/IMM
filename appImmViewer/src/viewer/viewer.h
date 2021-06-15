#pragma once

#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libRender/piRenderer.h"
#include "libCore/src/libSound/piSound.h"
#include "libCore/src/libBasics/piStr.h"
#include "libCore/src/libBasics/piTimer.h"
#include "libCore/src/libBasics/piTypes.h"
#include "libCore/src/libBasics/piWindow.h"

#include "libImmPlayer/src/player.h"

#include "../settings.h"
#include "piCameraD.h"


namespace ExePlayer
{
    class Viewer
    {
	public:
		typedef std::function<void(ImmPlayer::Player::LoadingState)> OnDocumentStateChange;

    private:
        int         mDocuments[4] = { -1, -1, -1, -1 }; // owned
        int         mLastChapter = -1;
        ImmCore::piTArray<bool> mChaptersViewed;

		ImmPlayer::Player::LoadingState mLastDocumentLoadingState;
		OnDocumentStateChange mOnLoadingStateChangeHandler;

        ImmCore::piCameraD   mCamera;       // owned
        ImmPlayer::Player   mPlayer;       // owned
        ImmCore::trans3d     mVRToHead;
		struct
		{
			ImmCore::mat4x4  mProjection;
			ImmCore::mat4x4  mInvProjection;
			ImmCore::trans3d mWorldToVR;
			ImmCore::trans3d mVRToWorld;
			ImmCore::trans3d mWorldToViewer;
			ImmCore::trans3d mViewerToWorld;
			ImmCore::trans3d mVRToViewer;
			ImmCore::trans3d mViewerToVR;
			ImmCore::vec3d   mCamPos; // in world
			ImmCore::vec3d   mCamDir; // in world
            ImmCore::trans3d   mSavedViewerToVR; // in VR
		}mViewerInfo;

        ImmCore::piRenderer* mRenderer;
        ImmCore::piLog* mLog;
        ImmCore::piSoundEngine* mSoundEngine;
        ImmPlayer::StereoMode mStereoMode;
        Settings* mPlayerSettings;
        ImmPlayer::Document::SpawnAreaInfo  mCurrentSpawnArea;
        int mCurrentSpawnAreaID;
        bool mInitialized = false;

        // Watch time tracking
		std::chrono::system_clock::time_point mLastViewTime;
		long mViewDurationInMs;
		void AccumulateViewTime();
		void MeasureEngagement();
		//

        bool mFilesLoaded;

        struct DisplayRenderState
        {
            struct
            {
                ImmCore::mat4x4 mEyeToClip;
                ImmCore::mat4x4 mViewerToEye; // in viewer space (viewer to eye)
                ImmCore::mat4x4 mViewerToClip;
                ImmCore::mat4x4 mClipToEye;
                ImmCore::mat4x4 mEyeToViewer;
                ImmCore::mat4x4 mClipToViewer;
            } mEye[2];
            ImmCore::mat4x4 mToolToViewer;
            ImmCore::vec2 mResolution;
        }mDisplayRenderState;
        ImmCore::piBuffer mDisplayStateShaderConstans;

        void iHandleDocumentInputs(ImmCore::piWindowEvents *evt, const bool vrEnabled, const ImmCore::piVRHMD::Controller *controller, const ImmCore::piVRHMD::Remote *remote, const ImmPlayer::Player::DocumentState & state);
        void iHandleGlobalInputs(ImmCore::piWindowEvents* evt, float dtime, const ImmCore::ivec2 & wndowSize);

    public:

        bool Init(const char *appID_DELETE_ME_THIS_IS_A_HACK, ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* soundEngine, ImmCore::piLog *log, ImmCore::piTimer *timer, ImmPlayer::StereoMode stereMode, Settings* playerSettings);
        void Deinit(void);

        void GlobalWork(ImmCore::piWindowEvents *evt, const bool vrEnabled, const ImmCore::trans3d & vr_to_head, const ImmCore::piVRHMD::Controller *controller, const ImmCore::piVRHMD::Remote *remote, ImmCore::piLog *log,
                        float dtime, const ImmCore::ivec2 & wndowSize, bool enabled, uint32_t microsecondsBudget, bool& isFirstFrame);

        void GlobalRender(const ImmCore::trans3d & vr_to_head, const ImmCore::vec4 & projection);
        void GlobalRender(const ImmCore::trans3d & vr_to_head, const ImmCore::mat4x4 & projection);

        void RenderStereoSinglePass(const ImmCore::ivec2 & pixelResolutionIncludingSupersampling,
            const ImmCore::trans3d & vr_to_head,
            const ImmCore::mat4x4d headToLEye, const ImmCore::vec4 & projectionLeft,
            const ImmCore::mat4x4d headToREye, const ImmCore::vec4 & projectionRight, piVRHMD* hmd);
        void RenderStereoSinglePass(const ImmCore::ivec2 & pixelResolutionIncludingSupersampling,
            const ImmCore::trans3d & vr_to_head,
            const ImmCore::mat4x4d headToLEye, const ImmCore::mat4x4 & projectionLeft,
            const ImmCore::mat4x4d headToREye, const ImmCore::mat4x4 & projectionRight, piVRHMD* hmd);

        void RenderStereoMultiPass(const ImmCore::ivec2 & pixelResolutionIncludingSupersampling,
            int passId,
            const ImmCore::mat4x4d headToEye, const ImmCore::vec4 & eyeProjection,
            const ImmCore::trans3d vr_to_head);
        void RenderStereoMultiPass( const ImmCore::ivec2 & pixelResolutionIncludingSupersampling,
            int passId,
            const ImmCore::mat4x4d headToEye, const ImmCore::mat4x4 & eyeProjection,
            const ImmCore::trans3d vr_to_head);

        void RenderMono(const ImmCore::ivec2 & pixelResolutionIncludingSupersampling, const ImmCore::trans3d & vr_to_head, int eyeID);

        const ImmPlayer::Player::PerformanceInfo & GetPerformanceInfoForFrame();
        int GetLoadTimeInMs();
        long GetViewTimeInMs();
        int GetChapterViewCount();

		bool ReplaceLoadingStateChangeListener(OnDocumentStateChange handler);
		bool IsDocumentLoaded(int docID) const;
        bool IsDocumentLoading(int docID) const;
        bool DidDocumentLoadFail(int docID) const;
        ImmPlayer::Player::LoadingState GetDocumentLoadState(int docID) const;

        void SkipBack(int docID) { mPlayer.SkipBack(docID); mLastChapter = -1; };
        void SkipForward(int docID) { mPlayer.SkipForward(docID); mLastChapter = -1; };
        void Restart(int docID) { mPlayer.Restart(docID); };
        void Continue(int docID) { mPlayer.Continue(docID);  }

        void Resume(int docID) { mPlayer.Resume(docID); mLastViewTime = std::chrono::system_clock::now(); };
        void Pause(int docID) { mPlayer.Pause(docID); }
        void Pause(int docID, uint64_t stopTicks) { mPlayer.Pause(docID, stopTicks); }
        void Resume(int docID, uint64_t startTicks) { mPlayer.Resume(docID, startTicks); mLastViewTime = std::chrono::system_clock::now(); }

        bool IsPaused(int docID) const;
        bool IsWaiting(int docID) const;

        void Unload(int docID);
        void UnloadAll();
        void UnloadAllSync();
        void SetSpawnArea(int docID, int spawnAreaID, bool recenter);
        //void CycleSpawnAreas(int docID);

        void CancelLoading(int docID);

        size_t GetChapterCount(int docID) { return mPlayer.GetChapterCount(docID); }
        size_t GetCurrentChapter(int docID) { return mPlayer.GetCurrentChapter(docID); }

        const ImmCore::piImage * GetSpawnAreaScreenshot(int docID, int spawnAreaID);
        int GetInitalSpawnArea(int docID);
        int GetSpawnAreaCount(int docID) { return mPlayer.GetSpawnAreaCount(0); }
        void TogglePlaybackState(int docID);

        void EnablePerformanceMeasurement(bool enabled) { mPlayer.EnablePerformanceMeasurement(enabled); }

        void SetVolume(int docID, float volume);
        bool GetVolume(int docID);

        bool HasPrev(int docID);

        void Prev(int docID);

        bool HasNext(int docID);

        void Next(int docID);

		ImmCore::trans3d GetWorldToVR();
    };

}
