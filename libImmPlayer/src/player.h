#pragma once

#include <mutex>

#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libRender/piRenderer.h"
#include "libCore/src/libSound/piSound.h"
#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libBasics/piTick.h"
#include "libCore/src/libBasics/piTimer.h"
#include "libCore/src/libBasics/piMutex.h"
#include "libCore/src/libBasics/piPool.h"

#include "layerRenderers/layerRendererPaint/layerRendererPaint.h"

#include "document.h"
#include "libImmImporter/src/document/layer.h"

namespace ImmPlayer {

    class Player
    {
    public:
        Player();
        ~Player();

        struct Configuration
        {
            ImmImporter::Drawing::ColorSpace  colorSpace;
            int         multisamplingLevel;
            DepthBuffer depthBuffer;
            ClipSpaceDepth clipDepth;
            ClipSpaceDepth projectionMatrix;
            bool           frontIsCCW;
            ImmImporter::Drawing::PaintRenderingTechnique paintRenderingTechnique = Drawing::Static;
        };

        bool Init(ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* sound, ImmCore::piLog* log, ImmCore::piTimer *timer, const Configuration * configuration);
        void Deinit(void);

        // call this only once per frame
        void GlobalWork( bool enabled, uint32_t microsecondsBudget);

        // call this ones per frame and per camera
        void GlobalRender(const ImmCore::trans3d & vr_to_head, const ImmCore::trans3d & world_to_head, const ImmCore::mat4x4 & projection, const StereoMode & stereoMode);

        //=== this are the only functions that can be called form the render thread. Everything else should always be called from the main thread ===
        // call this ones per frame and per camera, if in mono or two pass stereo
        void RenderMono(const ImmCore::ivec2 & pixelResolutionIncludingSupersampling, int eyeID);
        // call this ones per frame and per camera, if in single pass stereo
        void RenderStereoMultiPass(const ImmCore::ivec2 & pixelResolutionIncludingSupersampling, int eyeID, const ImmCore::mat4x4d & vr_to_eye, const ImmCore::mat4x4 & projection);
        // call this ones per frame and per camera, if in single pass stereo
        void RenderStereoSinglePass(const ImmCore::ivec2 & pixelResolutionIncludingSupersampling, const ImmCore::mat4x4d & vr_to_lEye, const ImmCore::mat4x4 & lProjection, const ImmCore::mat4x4d & vr_to_rEye, const ImmCore::mat4x4 & rProjection);
        //===============================


        struct PlayerInfo
        {
            struct
            {
                float mRed;
                float mGreen;
                float mBlue;
            }mBackgrundColor;
        };
        int  Load(const wchar_t* name);
        int  Load(ImmCore::piTArray<uint8_t>* imm, const wchar_t* name);

        void Unload(int id);
        void UnloadAll();
        void UnloadAllSync();

        void SetDocumentToWorld(int id, const ImmCore::trans3d & m);

        int GetChapterCount(int docId);
        int GetCurrentChapter(int docId);

        int  GetSpawnAreaCount(int docId);
        int  GetSpawnArea(int docId);
        int  GetInitialSpawnArea(int docId);
        void SetSpawnArea(int docId, int spawnAreaId);
        bool GetSpawnAreaNeedsUpdate(int docId);
        void SetSpawnAreaNeedsUpdate(int docId, bool state);
        const ImmCore::piImage* GetSpawnAreaScreenshot(int docId, int spawnAreaId);
        bool GetSpawnAreaInfo(Document::SpawnAreaInfo & info, int docId, int spawnAreaId);

        // playback control
        void SetTime(int id, ImmCore::piTick timeSinceStart, ImmCore::piTick timeSinceStop);
        void GetTime(int id, ImmCore::piTick* timeSinceStart, ImmCore::piTick* timeSinceStop);

        void Pause(int id);        // pause
        void Pause(int id, uint64_t stopTicks); // pause at stop tick
        void Resume(int id);       // resume
        void Resume(int id, uint64_t startTicks);
        void Hide(int id);        // pause
        void Show(int id);       // resume
        void SkipForward(int id);       // jump to next chapter
        void SkipBack(int id);         // jump to prev chapter
        void Restart(int id);      // restart the whole comic
        void Continue(int id);       // continue from a stop

        bool GetHasAudio(int id);
        float GetDocumentVolume(int id) const ; // get master volume in [0.0f, 1.0f]
        void SetDocumentVolume(int id, float volume); // set master volume in [0.0f, 1.0f]

        ImmCore::bound3d GetDocumentBBox(int id) const; // get bounding box from root layer

        struct PerformanceInfo
        {
            PerformanceInfo & operator=(const PerformanceInfo& info)
            {
                paintRenderingStrategy.InitCopy(&info.paintRenderingStrategy);
                cpuLoadTimeMS = info.cpuLoadTimeMS;
                numDrawCalls = info.numDrawCalls;
                numDrawCallsCulled = info.numDrawCallsCulled;
                numTriangles = info.numTriangles;
                numTrianglesCulled = info.numTrianglesCulled;
                gpuTimeAverageMs = info.gpuTimeAverageMs;
                return *this;
            }

            ImmCore::piString paintRenderingStrategy;
            int cpuLoadTimeMS = 0;
            int numDrawCalls = 0;
            int numDrawCallsCulled = 0;
            int numTriangles = 0;
            int numTrianglesCulled = 0;
            float gpuTimeAverageMs = 0;

            float totalGPUTimeAcrossFrames = 0;
            int numFramesMeasured = 0;
        };

        // Stats for this frame populated after Render*() method.
        const PerformanceInfo & GetPerformanceInfoForFrame() { return mCurrentPerfInfo; }
        int GetLoadTimeInMs();

        enum class LoadingState : int
        {
            Unloaded = 0,
            Loading = 1,
            Loaded = 2,
            Unloading = 3,
            Failed = 4
        };

        enum class PlaybackState : int
        {
            Playing = 0,		 // still, animation, comic:   music is on , animations are on  (if any),     rendering
            Paused = 1,			 // still, animation, comic:   music is off, animations are off (if any),     rendering
            PausedAndHidden = 2, // still, animation, comic:   music is off, animations are off (if any), NOT rendering
            Waiting = 3,		 //                   comic:   music is on,  animations are on          ,     rendering
            Finished = 4		 //                   comic:   ....
        };

        struct DocumentState
        {
            LoadingState  mLoadingState = LoadingState::Unloaded;
            PlaybackState mPlaybackState = PlaybackState::Playing;
        };

        enum class DocumentType : int
        {
            Still = 0,
            Animated = 1,
            Comic = 2
        };

        // TODO: this doesn't scale...
        enum class DocumentInfoFlags : uint32_t
        {
            MOVABLE = 1 << 0,
            DISPLAYABLE = 1 << 1,
            PLAYABLE = 1 << 2,
            NEXTABLE = 1 << 3,
            PREVABLE = 1 << 4,
            TIMEABLE = 1 << 5,
            SOUNDABLE = 1 << 6,
            BOUNDABLE = 1 << 7,
            GRABBABLE = 1 << 8,
            VIEWABLE = 1 << 9,
        };

        void GetDocumentState(DocumentState & state, int id) const;
        uint32_t GetDocumentInfoEx(int id) const;
        void GetPlayerInfo(PlayerInfo & info) const;
        void CancelLoading(int id);

        void EnablePerformanceMeasurement(bool enabled) { mEnablePerformanceMeasurement = enabled; }
		void GetChapterInfo(size_t& numChapters, ImmCore::piTArray<ImmCore::piTick>& chapterLengths, bool& hasPlays, int id);

    private:
        void iGlobalWorkLayer(Layer* la, float masterVolum);
        void iDisplayPreRenderLayer(Layer* la, const ImmCore::trans3d & parentLocation, float parentOpacity, const ImmCore::trans3d & worldToViewer);
        void iUnloadNotInTimeline(Layer* root, ImmCore::piTick now);
        ImmCore::mat4x4 iConvertProjectionMatrix(const ImmCore::mat4x4 & mat);


        void PopulateDisplayRenderPerfInfo();

        //-----------------------------------------------------------------------------------------------------
        // DATA
        //-----------------------------------------------------------------------------------------------------

        typedef struct
        {
            float mTime;
            int mFrameID;
            int mDummy1;
            int mDummy2;
        } FrameState;	// slot 0

        struct
        {
            ImmCore::mat4x4  mProjection;
            ImmCore::trans3d mWorldToHead;
            ImmCore::trans3d mHeadToWorld;
            ImmCore::trans3d mVRToHead;

            ImmCore::vec3d   mCamPos; // in world
            ImmCore::vec3d   mCamDir; // in world
        }mViewerInfo;

        typedef struct
        {
            struct
            {
                //mat4x4 mMatrix_Prj;
                //mat4x4 mHeadToEye; // in viewer space (viewer to eye)
                //mat4x4 mMatrix_CamPrj;
                //mat4x4 mInvMatrix_Prj;
                //mat4x4 mEyeToHead;
                //mat4x4 mInvMatrix_CamPrj;
                ImmCore::mat4x4 mViewerToEye_Prj;
            } mEye[2];
            ImmCore::vec2 mResolution;
            uint32_t mEyeIndex;
        } DisplayRenderState; // slot 4


                              // totally useless except for the Fallback StereoMode :(
        typedef struct
        {
            uint32_t mID;
            uint32_t dummy1;
            uint32_t dummy2;
            uint32_t dummy3;
        }PassState;		// slot 5

        typedef struct
        {
            uint64_t mBlueNoiseTexture;
        }GlobalResourcesState;

        ImmCore::piRenderer* mRenderer;
        ImmCore::piSoundEngine* mSoundEngine;
        ImmCore::piLog* mLog;
        ImmCore::piTimer *mTimer;
        uint32_t mFrame;
        bool     mAnyDocToRender;

        FrameState mFrameState;
        DisplayRenderState mDisplayRenderState;
        PassState    mPassState;

        ImmCore::piBuffer mFrameStateShaderConstans;
        ImmCore::piBuffer mDisplayStateShaderConstans;
        ImmCore::piBuffer mLayerStateShaderConstans;
        ImmCore::piBuffer mPassStateShaderConstans;
        ImmCore::piBuffer mGlobalResourcesConstans;
        ImmCore::piTexture mBlueNoise;

        ImmCore::piRasterState mRasterState;
        ImmCore::piBlendState  mBlendState;
        ImmCore::piDepthState  mDepthState;

        // state
        int        mSeed;
        Drawing::ColorSpace mColorSpace;
        DepthBuffer mDetphBufferMode;
        ClipSpaceDepth mClipDepthMode;
        ClipSpaceDepth mProjectionMatricesMode;
        ImmCore::piPool mDocuments;
        ImmCore::piTArray<bool> mSynced;

        struct Command
        {
            Document::Command mCommand;
            int mTarget;
        };
        ImmCore::piTArray<Command> mCommandList;
        //=========================

        LayerRendererPaint * mLayerPaintRender = nullptr;
        LayerRendererPicture mLayerRenderPicture;
        LayerRendererSound mLayerRenderSound;
        LayerRendererModel mLayerRenderModel;

        bool mEnabled;
        std::mutex   mMutex; // to synch the main thread and the render thread. TODO: remove it - use double buffered rendering
        ImmCore::piTick     mTime;

        #if defined(RENDER_BUDGET) || defined(MEASURE_GPU_TIME)
        uint32_t mMicrosecondsLastFrame;
        #endif

        #ifdef RENDER_BUDGET
        uint32_t mMicrosecondsBudget;
        int     mDeltaCapTrigger;
        #endif
        int     mDeltaCap;

        PerformanceInfo mCurrentPerfInfo;
        std::chrono::system_clock::time_point mCPULoadStartTimeMS;

        bool mEnablePerformanceMeasurement = false;

        ImmImporter::Drawing::PaintRenderingTechnique mPaintRenderingTechnique;
    };

}
