#pragma once

#include <mutex>

#include "libCore/src/libBasics/piMutex.h"

#include "layerRenderers/layerRendererPaint/layerRendererPaint.h"
#include "layerRenderers/layerRendererPicture/layerRendererPicture.h"
#include "layerRenderers/layerRendererSound/layerRendererSound.h"
#include "layerRenderers/layerRendererModel/layerRendererModel.h"
#include "mngrPlayer.h"
#include "renderMode.h"

#include "../../libImmImporter/src/document/sequence.h"
#include "../../libImmImporter/src/document/layerSpawnArea.h"

namespace ImmPlayer {

    class Document
    {
    public:
        Document();
        ~Document();

        enum class LoadingState : int
        {
            LoadingPending = 0,
            LoadingCPU = 1,
            LoadingSPU = 7,
            LoadingGPU = 2,
            LoadingComplete = 10,
            Loaded = 3, // playing
            UnloadingGPU = 4,
            UnloadingSPU = 11,
            UnloadingCPU = 5,
            UnloadingCompleted = 12,
            StopingLoadingAsync = 13,
        };

        enum class ErrorState : int
        {
            NoError = 0,
            FailedCPU = 1,
            FailedSPU = 2,
            FailedGPU = 3
        };

        enum class PlaybackState : int
        {
            Playing = 0,		// playing (any case not below)
            Paused = 1,			// paused as requested per user, all animations and sounds stopped
            PausedAndHidden = 2,
            Waiting = 3,		// story waiting for input to proceed, but animations and sounds still playing
            Finished = 4		// story reached the end
        };

        enum class ImportType : int
        {
            IMM_disk = 1,
            IMM_memory = 2
        };

        struct SpawnAreaInfo
        {
            const wchar_t *mName;
            int mVersion;
            bool mIsFloorLevel;
            ImmCore::trans3d mSpawnAreaToWorld;
            ImmImporter::LayerSpawnArea::Volume mVolume;
            bool mAnimated = false;
        };

        struct Command
        {
            enum class Type : int
            {
                None = 0,
                Load = 1,
                Unload = 2,
                SkipForward = 3,
                SkipBack = 4,
                Hide = 5,
                Show = 6,
                Pause = 7,
                Resume = 8,
                Restart = 9,
                Continue = 10
            }mType;
            ImmCore::piTArray<uint8_t>* mArrayArg = nullptr;
            ImmCore::piString mStrArg;
            uint64_t mIntArg;
            ImportType mFileType;
        };

        bool Init(const wchar_t* name, uint32_t id);
        void End(void);

        ImmImporter::Sequence *GetSequence(void);

        ImmCore::trans3d GetDocumentToWorld(void) const;
        void SetDocumentToWorld(const ImmCore::trans3d & m);

        LoadingState  GetLoadingState(void);
        ErrorState    GetErrorState(void);
        PlaybackState GetPlaybackState(void);

        bool UpdateStateCPU(
            LayerRendererSound *layerRenderSound,
            LayerRendererPaint *layerPaintRender,
            LayerRendererPicture *layerRenderPicture,
            LayerRendererModel *layerRendererModel,
            ImmImporter::Drawing::ColorSpace colorSpace,
            ImmImporter::Drawing::PaintRenderingTechnique renderingTechnique,
            ImmCore::piSoundEngine* soundEngine, ImmCore::piLog *log, const ImmCore::piTick time, const Command * command);
        bool UpdateStateGPU(LayerRendererPaint *layerPaintRender, LayerRendererPicture *layerRenderPicture, LayerRendererModel *layerRenderModel,
            ImmCore::piRenderer* renderer, ImmCore::piLog *log, ImmImporter::Drawing::ColorSpace colorSpace);

        int GetChapterCount(void) const;
        int GetCurrentChapter(void) const;

        bool GetHasAudio(void);

        void CancelLoading(void);

        float GetVolume(void) const;
        void SetVolume( float volume, ImmCore::piLog *log );

        ImmCore::bound3d GetBBox(void) const;

        int  GetSpawnAreaCount();
        int  GetSpawnArea();
        int  GetInitialSpawnArea();
        void SetSpawnArea(int spawnAreaId);
        const ImmCore::piImage* GetSpawnAreaScreenshot(int spawnAreaId);
        void GetSpawnAreaInfo(SpawnAreaInfo &info, int spawnAreaId);
        bool GetSpawnAreaNeedsUpdate();
        void SetSpawnAreaNeedsUpdate(bool state);

        void GetTime(ImmCore::piTick now, ImmCore::piTick* timeSinceStart, ImmCore::piTick* timeSinceStop);
        void SetTime(ImmCore::piTick now, ImmCore::piTick timeSinceStart, ImmCore::piTick timeSinceStop);

        void UnloadCPU(LayerRendererPaint * layerRendererPaint, LayerRendererPicture * layerRendererPicture, ImmCore::piLog * log);
		bool GetHasPlays(void) const;
        void SetCommandId(int id);
        int  GetCommandId(void);
    private:
        bool iLoadCPU(ImmCore::piLog *log, ImmCore::piSoundEngine* soundEngine,
            LayerRendererPaint *layerPaintRender,
            LayerRendererPicture *layerRenderPicture,
            LayerRendererSound * layerRendererSound,
            LayerRendererModel *layerRenderModel,
            ImmImporter::Drawing::ColorSpace colorSpace,
            ImmImporter::Drawing::PaintRenderingTechnique renderingTechnique);
        bool iLoadSPU(LayerRendererSound *layerRenderSound, ImmCore::piSoundEngine* soundEngine, ImmCore::piLog *log);
        bool iLoadGPU(LayerRendererPaint *layerPaintRender, LayerRendererPicture *layerRenderPicture, LayerRendererModel *layerRenderModel, ImmCore::piRenderer* renderer,
            ImmCore::piLog *log, ImmImporter::Drawing::ColorSpace colorSpace);
        void iUnloadCPU(ImmCore::piLog *log, LayerRendererPaint *layerPaintRender, LayerRendererPicture *layerRenderPicture);
        bool iUnloadSPU(LayerRendererSound *layerRenderSound, ImmCore::piSoundEngine* soundEngine, ImmCore::piLog *log);
        bool iUnloadGPU(LayerRendererPaint *layerPaintRender, LayerRendererPicture *layerRenderPicture, LayerRendererModel *layerRenderModel, ImmCore::piRenderer* renderer, ImmCore::piLog *log);

        ImmCore::trans3d      mDocumentToWorld;
        ImmCore::piString     mFileName;
        ImportType mFileType;
        ImmCore::piTArray<uint8_t>* mIMM;
        ImmImporter::Sequence     mSequence;
        MngrPlayer   mPlayerManager;
        float        mMasterVolume;
        bool         mHidden;
        bool         mWasPaused;
        bool         mHasAudio = false;
        int          mID;
        int          mCmdID;
        struct
        {
            ErrorState    mErrorState;
            LoadingState  mLoadingState;
            PlaybackState mPlaybackState;
            ImmCore::piMutex       mMutex;
        }mState;
    };

}
