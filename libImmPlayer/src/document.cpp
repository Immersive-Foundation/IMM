#include <malloc.h>
#include <thread>
#include <chrono>


#include "libCore/src/libBasics/piDebug.h"

#include "libImmImporter/src/document/layer.h"
#include "libImmImporter/src/document/layerEffect.h"
#include "libImmImporter/src/document/layerInstance.h"
#include "libImmImporter/src/document/layerPaint.h"
#include "libImmImporter/src/document/layerSound.h"
#include "libImmImporter/src/document/layerSpawnArea.h"
#include "libImmImporter/src/document/sequence.h"
#include "libImmImporter/src/fromImmersive/fromImmersive.h"

#include "document.h"
using namespace ImmCore;
using namespace ImmImporter;


namespace ImmPlayer
{
    Document::Document() {}

    Document::~Document() {}

    bool Document::Init(const wchar_t* name, uint32_t id)
    {
        //mLog = log;

        mID = id;

        if (!mState.mMutex.Init())
            return false;

        if (!mFileName.Init(1024))
            return false;

        mState.mLoadingState = LoadingState::LoadingPending;
        mState.mErrorState = ErrorState::NoError;
        mState.mPlaybackState = PlaybackState::Waiting;
        mHidden = false;
        mDocumentToWorld = trans3d::identity();
        mMasterVolume = 1.0f;
        mCmdID = -1;
        return true;
    }

    void Document::End(void)
    {
        mFileName.End();
        mState.mMutex.End();
    }

    trans3d Document::GetDocumentToWorld(void)const
    {
        return mDocumentToWorld;
    }

    void Document::SetDocumentToWorld(const trans3d & m)
    {
        mDocumentToWorld = m;
    }


    int Document::GetSpawnAreaCount()
    {
        return mPlayerManager.GetSpawnAreaCount();
    }

    int Document::GetSpawnArea()
    {
        return mPlayerManager.GetSpawnArea();
    }

    int Document::GetInitialSpawnArea()
    {
        return mPlayerManager.GetInitialSpawnArea();
    }

    void Document::SetSpawnArea(int spawnAreaId)
    {
        mPlayerManager.SetSpawnArea(spawnAreaId);
    }

    const piImage* Document::GetSpawnAreaScreenshot(int spawnAreaId)
    {
        const Layer *layer = mPlayerManager.GetSpawnAreaLayer(spawnAreaId);
        const LayerSpawnArea* layerSpawnArea = (LayerSpawnArea*)layer->GetImplementation();

        return layerSpawnArea->GetScreenshot();
    }

    void Document::GetSpawnAreaInfo(SpawnAreaInfo& info, int spawnAreaId)
    {
        const Layer *layer = mPlayerManager.GetSpawnAreaLayer(spawnAreaId);
        const LayerSpawnArea* layerSpawnArea = (LayerSpawnArea*)layer->GetImplementation();
        info.mName = layer->GetName().GetS();
        info.mVersion = layerSpawnArea->GetVersion();
        info.mIsFloorLevel = layerSpawnArea->GetTracking() == LayerSpawnArea::TrackingLevel::Floor;
        info.mSpawnAreaToWorld = layer->GetTransformToWorld();
        info.mVolume = layerSpawnArea->GetVolume();

        const bool isSelfAnimated = layer->GetNumAnimKeys(Layer::AnimProperty::Transform) > 1;
        bool areParentsAnimated = false;
        Layer* parent = layer->GetParent();
        while (parent != nullptr)
        {
            if (parent->GetNumAnimKeys(Layer::AnimProperty::Transform) > 1)
            {
                areParentsAnimated = true;
                break;
            }
            parent = parent->GetParent();
        }
        info.mAnimated = isSelfAnimated || areParentsAnimated;
    }

    bool Document::GetSpawnAreaNeedsUpdate()
    {
        return mSequence.GetSpawnAreaNeedsUpdate();
    }

    void Document::SetSpawnAreaNeedsUpdate(bool state)
    {
        mSequence.SetSpawnAreaNeedsUpdate(state);
    }

    void Document::GetTime(piTick now, piTick * timeSinceStart, piTick * timeSinceStop)
    {
        mPlayerManager.GetTime(now, timeSinceStart, timeSinceStop);
    }

    void Document::SetTime(piTick now, piTick timeSinceStart, piTick timeSinceStop)
    {
        mPlayerManager.SetTime(now, timeSinceStart, timeSinceStop);
    }

    //----------------------------------------------------

    Document::PlaybackState Document::GetPlaybackState(void)
    {
        return mState.mPlaybackState;
    }

    Sequence *Document::GetSequence(void)
    {
        return &mSequence;
    }


    Document::LoadingState Document::GetLoadingState(void)
    {
        LoadingState res;
        mState.mMutex.Lock();
        res = mState.mLoadingState;
        mState.mMutex.UnLock();
        return res;
    }

    Document::ErrorState Document::GetErrorState(void)
    {
        ErrorState res;
        mState.mMutex.Lock();
        res = mState.mErrorState;
        mState.mMutex.UnLock();
        return res;
    }

    //----------------------------------------------------

    bool Document::UpdateStateCPU(
        LayerRendererSound *layerRenderSound,
        LayerRendererPaint *layerPaintRender,
        LayerRendererPicture *layerRenderPicture,
        LayerRendererModel *layerRendererModel,
        Drawing::ColorSpace colorSpace,
        Drawing::PaintRenderingTechnique renderingTechnique,
        piSoundEngine* soundEngine, piLog *log, const piTick now,
        const Command * command)
    {
        //======================================================================
        // 1. process loading commands
        //======================================================================

        if (command != nullptr)
        {
            if (command->mType == Command::Type::Load)
            {
                if (mState.mLoadingState == Document::LoadingState::LoadingPending || mState.mLoadingState == Document::LoadingState::UnloadingCompleted)
                {
                    mState.mLoadingState = LoadingState::LoadingPending;
                    mIMM = command->mArrayArg;
                    if (!mFileName.Copy(&command->mStrArg))
                        return false;
                    mFileType = command->mFileType;
                }
            }
            else if (command->mType == Command::Type::Unload)
            {
                if (mState.mLoadingState == Document::LoadingState::Loaded)
                {
                    if (IsLoadingAsync())
                    {
                        StopLoadingAsync();
                        mState.mLoadingState = LoadingState::StopingLoadingAsync;
                    }
                    else
                    {
                        mPlayerManager.Exit();
                        mState.mLoadingState = LoadingState::UnloadingGPU;
                    }
                }
            }
        }

        //======================================================================
        // 2. process loading state machine
        //======================================================================

        const LoadingState st = mState.mLoadingState;
        if (st == LoadingState::LoadingPending)
        {
            mState.mLoadingState = LoadingState::LoadingCPU;

            std::thread loadingThread([this, log, soundEngine, layerPaintRender, layerRenderPicture, layerRenderSound, layerRendererModel, colorSpace, renderingTechnique]()
            {
                if (!iLoadCPU(log, soundEngine, layerPaintRender, layerRenderPicture, layerRenderSound, layerRendererModel, colorSpace, renderingTechnique))
                {
                    mState.mLoadingState = LoadingState::UnloadingCompleted;
                    mState.mErrorState = ErrorState::FailedCPU;
                }
                else
                {
                    mState.mLoadingState = LoadingState::LoadingSPU;
                }
            });

            loadingThread.detach();
        }
        else if (st == LoadingState::LoadingCPU)
        {
            // do nothing, keep waiting
        }
        else if (st == LoadingState::LoadingSPU)
        {
            //LoadSPU(&mLayerRenderSound, mSoundEngine, mLog);
            if (!iLoadSPU(layerRenderSound, soundEngine, log))
            {
                mState.mLoadingState = LoadingState::UnloadingCPU;
                mState.mErrorState = ErrorState::FailedSPU;
            }
            else
            {
                mState.mLoadingState = LoadingState::LoadingGPU;
            }
        }
        else if (st == LoadingState::LoadingGPU)
        {
            // do nothing, handled in UpdateStateGPU
        }
        else if (st == LoadingState::LoadingComplete)
        {
            // after a new document is loaded
            // update the viewer position in player to rencenter
            // the player needs to save this to do origin calibration when changing spawn areas
            mPlayerManager.Enter(&mSequence, now );
            mState.mLoadingState = LoadingState::Loaded;
        }
        else if (st == LoadingState::Loaded)
        {
        }
        else if (st == LoadingState::StopingLoadingAsync)
        {
            // wait for the loading thread to finish
            if (!IsLoadingAsync())
            {
                mPlayerManager.Exit();
                mState.mLoadingState = LoadingState::UnloadingGPU;
            }
        }
        else if (st == LoadingState::UnloadingGPU)
        {
            // do nothing, handled in UpdateStateGPU
        }
        else if (st == LoadingState::UnloadingSPU)
        {
            iUnloadSPU(layerRenderSound, soundEngine, log);
            mState.mLoadingState = LoadingState::UnloadingCPU;
        }
        else if (st == LoadingState::UnloadingCPU)
        {
            iUnloadCPU(log, layerPaintRender, layerRenderPicture);
            mState.mLoadingState = LoadingState::UnloadingCompleted;
        }
        else if (st == LoadingState::UnloadingCompleted)
        {
            // do nothing
        }



        //======================================================================
        // 3. process playback commands
        //======================================================================
        if (command != nullptr)
        {
            if (command->mType == Command::Type::SkipForward)
            {
                if (mState.mLoadingState == LoadingState::Loaded)
                {
                    mPlayerManager.SkipForward(now);
                }
            }
            else if (command->mType == Command::Type::SkipBack)
            {
                if (mState.mLoadingState == LoadingState::Loaded)
                {
                    mPlayerManager.SkipBack(now);
                }
            }
            else if (command->mType == Command::Type::Pause)
            {
                if (mState.mLoadingState == LoadingState::Loaded && !mHidden)
                {
                    const bool isPaused = mPlayerManager.GetIsPaused();
                    if (!isPaused)
                    {
                        if (command->mIntArg != 0)
                        {
                            mPlayerManager.Pause(piTick(static_cast<int64_t>(command->mIntArg)));
                        }
                        else
                        {
                            mPlayerManager.Pause(now);
                        }
                    }
                }
            }
            else if (command->mType == Command::Type::Resume)
            {
                if (mState.mLoadingState == LoadingState::Loaded && !mHidden)
                {
                    const bool isPaused = mPlayerManager.GetIsPaused();
                    if (isPaused)
                    {
                        if (command->mIntArg != 0)
                        {
                            mPlayerManager.Resume(piTick(static_cast<int64_t>(command->mIntArg)));
                        }
                        else
                        {
                            mPlayerManager.Resume(now);
                        }
                    }
                }
            }
            else if (command->mType == Command::Type::Show)
            {
                if (mState.mLoadingState == LoadingState::Loaded)
                {
                    mHidden = false;
                    if( !mWasPaused )
                    {
                        mPlayerManager.Resume(now);
                    }
                }
            }
            else if (command->mType == Command::Type::Hide)
            {
                if (mState.mLoadingState == LoadingState::Loaded)
                {
                    const bool isPaused = mPlayerManager.GetIsPaused();
                    if (!isPaused)
                    {
                        mPlayerManager.Pause(now);
                    }
                    mWasPaused = isPaused;
                    mHidden = true;
                }
            }
            else if (command->mType == Command::Type::Continue)
            {
                if (mState.mLoadingState == LoadingState::Loaded)
                {
                    mPlayerManager.Continue(now);
                }
            }
            else if (command->mType == Command::Type::Restart)
            {
                if (mState.mLoadingState == LoadingState::Loaded)
                {
                    mPlayerManager.Restart(now);
                }
            }
        }


        {
            if (mState.mLoadingState != LoadingState::Loaded) mState.mPlaybackState = PlaybackState::PausedAndHidden;
            else if (mHidden) mState.mPlaybackState = PlaybackState::PausedAndHidden;
            else if (mPlayerManager.GetIsFinished()) mState.mPlaybackState = PlaybackState::Finished;  // story has reached the end, need to check this before waiting!!!
            else if (mPlayerManager.GetIsPaused()) mState.mPlaybackState = PlaybackState::Paused;
            else if (mPlayerManager.GetIsWaiting()) mState.mPlaybackState = PlaybackState::Waiting;   // story driven pause/wait to proceed, still playing
            else
                mState.mPlaybackState = PlaybackState::Playing;
        }

        const bool shouldRender = mState.mLoadingState == Document::LoadingState::Loaded && mState.mPlaybackState != Document::PlaybackState::PausedAndHidden;


        //======================================================================
        // 4. animation
        //======================================================================

        if (shouldRender)
        {
            if (mPlayerManager.GetIsPlaying())
                mPlayerManager.Update(now);
        }


        // should we animate+render?
        return shouldRender;
    }

    bool Document::UpdateStateGPU(LayerRendererPaint *layerPaintRender,  LayerRendererPicture *layerRenderPicture, LayerRendererModel *layerRenderModel,
        piRenderer* renderer, piLog *log, Drawing::ColorSpace colorSpace)
    {
        Document::LoadingState st = mState.mLoadingState;
        if (st == LoadingState::LoadingGPU)
        {
            if (!iLoadGPU(layerPaintRender, layerRenderPicture, layerRenderModel, renderer, log, colorSpace))
            {
                mState.mLoadingState = LoadingState::UnloadingSPU;
                mState.mErrorState = ErrorState::FailedGPU;
            }
            else
            {
                mState.mLoadingState = LoadingState::LoadingComplete;
            }
        }
        else if (st == LoadingState::UnloadingGPU)
        {
            iUnloadGPU(layerPaintRender, layerRenderPicture, layerRenderModel, renderer, log);
            mState.mLoadingState = LoadingState::UnloadingSPU;
        }

        // should we animate+render?
        return (mState.mLoadingState == Document::LoadingState::Loaded && mState.mPlaybackState != Document::PlaybackState::PausedAndHidden);
    }

    //===============================================
    bool Document::iLoadSPU(LayerRendererSound *layerRenderSound, piSoundEngine* soundEngine, piLog *log)
    {
        std::chrono::steady_clock::time_point timeStart = std::chrono::steady_clock::now();

        log->Printf(LT_MESSAGE, L"Loading in SPU...");

        auto loadInSPU = [this, log, layerRenderSound, soundEngine](Layer* layer, int level, int child, bool instance) -> bool
        {
            if (layer->GetType() == Layer::Type::Sound)
            {
                if (!mHasAudio)
                    mHasAudio = true;
                if (!layerRenderSound->LoadInCPU(log, layer))
                {
                    log->Printf(LT_ERROR, L"Could not load in CPU Sound layer %s", layer->GetFullName()->GetS());
                    return false;
                }
#ifndef UNLOAD_SOUNDS
                // When voice management is on, we don't preload sounds in the sound engine
                if (!layerRenderSound->LoadInSPU(soundEngine, log, layer))
                {
                    log->Printf(LT_ERROR, L"Could not load in SPU Sound layer %s", layer->GetFullName()->GetS());
                    return false;
                }
#endif
            }
            return true;
        };
        if (!mSequence.Recurse(loadInSPU, false, false, false, false))
        {
            log->Printf(LT_ERROR, L"Error loading in SPU...");
            return false;
        }

        std::chrono::steady_clock::time_point timeEnd = std::chrono::steady_clock::now();

        log->Printf(LT_MESSAGE, L"Loaded in SPU! (%d ms)", static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count()));

        return true;
    }

    bool Document::iUnloadSPU(LayerRendererSound *layerRenderSound, piSoundEngine* soundEngine, piLog *log)
    {
        std::chrono::steady_clock::time_point timeStart = std::chrono::steady_clock::now();

        log->Printf(LT_MESSAGE, L"Unloading SPU [%d]...", mID);

        auto unloadInSPU = [this, log, layerRenderSound, soundEngine](Layer* layer, int level, int child, bool instance) -> bool
        {
            if (layer->GetType() == Layer::Type::Sound)
            {
                if (!layerRenderSound->UnloadInSPU(soundEngine, layer))
                {
                    log->Printf(LT_ERROR, L"Could not unload in SPU Sound layer %s", layer->GetFullName()->GetS());
                    //return false;
                }
            }
            return true;
        };

        if (!mSequence.Recurse(unloadInSPU, false, false, false, false))
        {
            log->Printf(LT_ERROR, L"Error unloading in SPU...");
            return false;
        }

        std::chrono::steady_clock::time_point timeEnd = std::chrono::steady_clock::now();

        log->Printf(LT_MESSAGE, L"Unloaded in SPU! (%d ms)", static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count()));

        return true;
    }

    bool Document::iLoadCPU( piLog *log, piSoundEngine* soundEngine,
        LayerRendererPaint *layerPaintRender,
        LayerRendererPicture *layerRenderPicture,
        LayerRendererSound * layerRendererSound,
        LayerRendererModel *layerRenderModel,
        Drawing::ColorSpace colorSpace,
        Drawing::PaintRenderingTechnique renderingTechnique)
    {
        std::chrono::steady_clock::time_point timeStart = std::chrono::steady_clock::now();

        log->Printf(LT_MESSAGE, L"Loading in CPU...");

        //----------------------
        // sequence
        //----------------------

        switch (mFileType)
        {
        case ImportType::IMM_disk:
        {
            if (!ImportFromDisk(&mSequence, log, mFileName.GetS(), colorSpace, renderingTechnique))
            {
                log->Printf(LT_ERROR, L"Could not load IMM from disk : %s", mFileName.GetS());
                return false;
            }
            break;
        }
        case ImportType::IMM_memory:
        {
            if (!ImportFromMemory(mIMM, &mSequence, log, colorSpace, renderingTechnique))
            {
                log->Printf(LT_ERROR, L"Could not load IMM from memory");
                return false;
            }
            break;
        }
        }

        //----------------------
        //
        //----------------------
        if (!mPlayerManager.Init(log))
            return false;

        //----------------------
        // layer renderers
        //----------------------

        if (!mSequence.Recurse(
            [this, log, layerPaintRender, layerRenderPicture, layerRenderModel](Layer* layer, int level, int child, bool instance) -> bool
            {
                if (IsStoppedLoading()) return false;

                const Layer::Type layerType = layer->GetType();
                bool res = true;
                if (layerType == Layer::Type::Paint)   res = layerPaintRender->LoadInCPU(log, layer);
                if (layerType == Layer::Type::Picture) res = layerRenderPicture->LoadInCPU(log, layer);
                if (layerType == Layer::Type::Model) res = layerRenderModel->LoadInCPU(log, layer);
                if (!res) log->Printf(LT_ERROR, L"Could not load LayerRenderer [%d] for layer %s", mID, layer->GetFullName()->GetS());
                return res;
            }, false, false, false, false))
                return false;

        //----------------------

        std::chrono::steady_clock::time_point timeEnd = std::chrono::steady_clock::now();

        log->Printf(LT_MESSAGE, L"Loaded in CPU! (%d ms), file: %s", static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count()), mFileName.GetS());

        return true;
    }

    void Document::iUnloadCPU(piLog *log,
        LayerRendererPaint *layerPaintRender,
        LayerRendererPicture *layerRenderPicture)
    {
        std::chrono::steady_clock::time_point timeStart = std::chrono::steady_clock::now();

        // rendering relevant CPU data
        mSequence.Recurse(
            [this, log, layerPaintRender, layerRenderPicture](Layer* layer, int level, int child, bool instance) -> bool
        {
            const Layer::Type layerType = layer->GetType();
            if (layerType == Layer::Type::Paint)   layerPaintRender->UnloadInCPU(log, layer);
            if (layerType == Layer::Type::Picture) layerRenderPicture->UnloadInCPU(log, layer);
            return true;
        }, false, false, false, false);

        // player
        mPlayerManager.Deinit();

        // sequence
        mSequence.Deinit(log);

        std::chrono::steady_clock::time_point timeEnd = std::chrono::steady_clock::now();

        log->Printf(LT_MESSAGE, L"Unloaded in CPU [%d]! (%d ms)", mID, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count()));
    }


    bool Document::iLoadGPU(LayerRendererPaint *layerPaintRender,
        LayerRendererPicture *layerRenderPicture,
        LayerRendererModel *layerRenderModel,
        piRenderer* renderer,
        piLog *log, Drawing::ColorSpace colorSpace)
    {
        std::chrono::steady_clock::time_point timeStart = std::chrono::steady_clock::now();

        log->Printf(LT_MESSAGE, L"Loading in GPU [%d]...", mID);
        auto loadInGPU = [this, colorSpace, renderer, log, layerPaintRender, layerRenderPicture, layerRenderModel](Layer* layer, int level, int child, bool instance) -> bool {
            const Layer::Type layerType = layer->GetType();
            switch (layerType)
            {
            case Layer::Type::Paint:
                if (!layerPaintRender->LoadInGPU(renderer, nullptr, log, layer))
                {
                    log->Printf(LT_ERROR, L"Could not load in GPU [%d] Paint layer %s", mID, layer->GetFullName()->GetS());
                    return false;
                }
                break;
            case Layer::Type::Picture:
                if (!layerRenderPicture->LoadInGPU(renderer, nullptr, log, layer))
                {
                    log->Printf(LT_ERROR, L"Could not load in GPU [%d] Picture layer %s", mID, layer->GetFullName()->GetS());
                    return false;
                }
                break;
            case Layer::Type::Model:
                if (!layerRenderModel->LoadInGPU(renderer, nullptr, log, layer))
                {
                    log->Printf(LT_ERROR, L"Could not load in GPU [%d] Model layer %s", mID, layer->GetFullName()->GetS());
                    return false;
                }
                break;
            default:
                break;
            }
            return true;
        };
        if (!mSequence.Recurse(loadInGPU, false, false, false, false))
        {
            log->Printf(LT_ERROR, L"Error loading in GPU [%d]...", mID);
            return false;
        }

        std::chrono::steady_clock::time_point timeEnd = std::chrono::steady_clock::now();

        const int ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count());
        log->Printf(LT_MESSAGE, L"Loaded in GPU [%d]! (%d ms)", mID, ms);

        return true;
    }

    bool Document::iUnloadGPU(LayerRendererPaint *layerPaintRender,
        LayerRendererPicture *layerRenderPicture,
        LayerRendererModel *layerRenderModel,
        piRenderer* renderer, piLog *log)
    {
        std::chrono::steady_clock::time_point timeStart = std::chrono::steady_clock::now();

        log->Printf(LT_MESSAGE, L"Unloading GPU [%d]...", mID);

        auto unloadInGPU = [this, renderer, log, layerPaintRender, layerRenderPicture, layerRenderModel ](Layer* layer, int level, int child, bool instance) -> bool {
            const Layer::Type layerType = layer->GetType();
            switch (layerType)
            {
            case Layer::Type::Paint:
                if (!layerPaintRender->UnloadInGPU(renderer, nullptr, log, layer)) return false;
                break;
            case Layer::Type::Picture:
                if (!layerRenderPicture->UnloadInGPU(renderer, nullptr, log, layer)) return false;
                break;
            case Layer::Type::Model:
                if (!layerRenderModel->UnloadInGPU(renderer, nullptr, log, layer)) return false;
                break;
            default:
                break;
            }
            return true;
        };
        if (!mSequence.Recurse(unloadInGPU, false, false, false, false))
        {
            log->Printf(LT_ERROR, L"Error loading in GPU [%d]...", mID);
            return false;
        }



        std::chrono::steady_clock::time_point timeEnd = std::chrono::steady_clock::now();

        log->Printf(LT_MESSAGE, L"Unloaded in GPU [%d]! (%d ms)", mID, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count()));

        return true;
    }

    int Document::GetChapterCount(void) const
    {
        return static_cast<int>(mPlayerManager.GetChapterCount());
    }

    int Document::GetCurrentChapter(void) const
    {
        return static_cast<int>(mPlayerManager.GetCurrentChapter());
    }

	bool Document::GetHasPlays(void) const
	{
		return mPlayerManager.GetHasPlays();
	}

    bool Document::GetHasAudio(void)
    {
        return mHasAudio;
    }

    void Document::CancelLoading(void)
    {
        StopLoadingAsync();
    }

    float Document::GetVolume(void) const
    {
        return mMasterVolume;
    }

    void Document::SetVolume( float volume, piLog *log )
    {
        // clamp01
        if (volume > 1.0f) volume = 1.0f;
        if (volume < 0.0f) volume = 0.0f;

        mMasterVolume = volume;
    }

    bound3d Document::GetBBox(void) const
    {
        bound3d bbox = mSequence.GetRoot()->GetBBox();
        bbox = btransform(bbox, GetDocumentToWorld());
        return bbox;
    }

    void Document::UnloadCPU(LayerRendererPaint * layerRendererPaint, LayerRendererPicture * layerRendererPicture, piLog * log)
    {
        iUnloadCPU(log, layerRendererPaint, layerRendererPicture);
    }

    void Document::SetCommandId(int id)
    {
        mCmdID = id;
    }

    int Document::GetCommandId(void)
    {
        return mCmdID;
    }
}
