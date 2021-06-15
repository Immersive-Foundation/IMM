#include "libImmImporter/src/document/layer.h"
#include "libImmImporter/src/document/sequence.h"
#include "libImmImporter/src/document/layerSound.h"
#include "libImmImporter/src/document/layerSpawnArea.h"

#include "mngrPlayer.h"
using namespace ImmCore;
using namespace ImmImporter;

namespace ImmPlayer
{
    MngrPlayer::MngrPlayer()
    {
    }

    MngrPlayer::~MngrPlayer()
    {
    }

    bool MngrPlayer::Init(piLog * log)
    {
        mIsPlaying = false;
        mIsWaitingForInput = false;
        mIsPaused = false;
        mDocument = nullptr;
        mLog = log;
        mIsAtEnd = false;
        mCurrentSpawnAreaId = -1;
        mHasLoopKey = false;
        mLoopKeyTime = piTick::FromSeconds(60.0*60.0*1000.0);
        if (!mSpawnAreas.Init(6, true))
            return false;

        return true;
    }


    void MngrPlayer::Deinit()
    {
        mSpawnAreas.End();
    }


    void MngrPlayer::Enter(Sequence * document, const piTick now)
    {
        mDocument = document;
#ifdef _DEBUG
        mLog->Printf(LT_DEBUG, L"Enter!");
#endif
        mIsPlaying = true;
        mIsAtEnd = false;

        // Parse and set initial spawn area
        auto saveSpawnArea = [this](Layer* layer, int level, int child, bool instance) -> bool
        {
            if (layer->GetType() == Layer::Type::SpawnArea)
            {
                LayerSpawnArea * lv = (LayerSpawnArea*)layer->GetImplementation();
                mSpawnAreas.Append(layer, true);
            }
            return true;
        };
        mSpawnAreas.Reset();
        mDocument->Recurse(saveSpawnArea, false, false, false, false);

        const Layer *layer = mDocument->GetInitialSpawnArea();

        if( layer!=nullptr )
        {
            int initialSpawnAreaId = FindSpawnAreaIdByLayer(layer);
            if (initialSpawnAreaId == -1)
            {
                mLog->Printf(LT_ERROR, L"Could not find default spawn area, setting to 0.");
                initialSpawnAreaId = 0;
            }

            SetSpawnArea(initialSpawnAreaId);
        }

        // Count root plays and stops
        Layer* root = mDocument->GetRoot();
        const unsigned int numActionKeys = root->GetNumAnimKeys(Layer::AnimProperty::Action);
        mCurrentChapter = 0;
        mNumPlayMarkers = 0;
        mNumStopMarkers = 0;
        for (unsigned int i = 0; i < numActionKeys; i++)
        {
            Layer::AnimKey* key = (Layer::AnimKey*) root->GetAnimKey(Layer::AnimProperty::Action, i);
            Layer::AnimAction action = static_cast<Layer::AnimAction>(key->mValue.mInt);
            if (action == Layer::AnimAction::Stop)
                mNumStopMarkers++;
            else if (action == Layer::AnimAction::Play)
                mNumPlayMarkers++;
            else if (action == Layer::AnimAction::Loop)
            {
                mHasLoopKey = true;
                mLoopKeyTime = key->mTime;
            }
        }

        mNumChapters = mNumPlayMarkers > 0 ? (mNumPlayMarkers + 1) : (mNumStopMarkers + 1);

        mLog->Printf(LT_MESSAGE, L"Has %d chapters", mNumChapters);

        Restart(now);
    }

	bool MngrPlayer::GetHasPlays(void) const
	{
		return mNumPlayMarkers > 0;
	}

    void MngrPlayer::Restart(const piTick now)
    {
        mIsWaitingForInput = false;
        mIsAtEnd = false;
        mIsPaused = false;

        // Start playback of root
#ifdef _DEBUG
        mLog->Printf(LT_DEBUG, L"Restart");
#endif

        iJumpAndReset(now, piTick(0));
    }

    void MngrPlayer::Exit()
    {
#ifdef _DEBUG
        mLog->Printf(LT_DEBUG, L"Exit");
#endif
        Layer* root = mDocument->GetRoot();
        mIsPlaying = false;
        root->SetPlaying(false);
    }

    void MngrPlayer::Pause(const piTick now)
    {
        if (mIsPaused) return;

#ifdef _DEBUG
        mLog->Printf(LT_DEBUG, L"Pause");
#endif
        Layer* root = mDocument->GetRoot();

        // Update all layer states
        auto update = [this, &now, &root](Layer* layer, int level, int child, bool instance) -> bool
        {
            switch (layer->GetType())
            {
            case Layer::Type::Sound:
            {
                LayerSound * ls = (LayerSound*)layer->GetImplementation();
                if (ls->GetPlaying())
                {
                    //ls->SetPlaying(false);
					ls->SetPaused(true);
                    //layer->SetVisible(false);
                }
                break;
            }

            case Layer::Type::Paint:
            {
                LayerPaint * lp = (LayerPaint*)layer->GetImplementation();
                lp->SetPlaying(false);
                break;
            }
            case Layer::Type::Group:
                // do not restop root if already waiting for input
                if (layer == root && mIsWaitingForInput)
                    break;
                // stop timelines
                if (layer->GetIsTimeline() && layer->GetPlaying())
                {
                    layer->SetPlaying(false);
                    layer->SetStopTime(now);
                }
                break;

            default:
                break;
            }
            return true;
        };

        mDocument->Recurse(update, false, false, false, false);

        mIsPaused = true;
    }

    void MngrPlayer::Resume(const piTick now)
    {
        if (!mIsPaused) return;

#ifdef _DEBUG
        mLog->Printf(LT_DEBUG, L"Resume");
#endif
        Layer* root = mDocument->GetRoot();

        auto update = [this,&now, &root](Layer* layer, int level, int child, bool instance) -> bool
        {
            switch (layer->GetType())
            {
            case Layer::Type::Sound:

			{
				LayerSound * ls = (LayerSound*)layer->GetImplementation();
				ls->SetPaused(false);
                break;
			}

            case Layer::Type::Paint:
            {
                LayerPaint * lp = (LayerPaint*)layer->GetImplementation();
                lp->SetPlaying(true);
                break;
            }
            case Layer::Type::Group:
                // do not restart root if waiting for input
                if (layer == root && mIsWaitingForInput)
                    break;
                // restart timelines
                if (layer->GetWorldVisible() && layer->GetIsTimeline() && ! layer->GetPlaying())
                {
                    layer->SetPlaying(true);
                    const piTick currentPosition = layer->GetStopTime() - layer->GetStartTime();
                    layer->SetStartTime(now - currentPosition );
                    layer->SetStopTime(now - currentPosition - 1);
                }
                break;

            default:
                break;
            }
            return true;
        };
        mDocument->Recurse(update, false, false, false, false);

        mIsPaused = false;
    }

    int MngrPlayer::GetSpawnAreaCount() const
    {
        return (int)mSpawnAreas.GetLength();
    }

    int MngrPlayer::GetSpawnArea()
    {
        return mCurrentSpawnAreaId;
    }

    int MngrPlayer::GetInitialSpawnArea()
    {
        const Layer *initialSpawnArea = mDocument->GetInitialSpawnArea();
        return FindSpawnAreaIdByLayer(initialSpawnArea);
    }

    void MngrPlayer::SetSpawnArea(int spawnAreaId)
    {
        if (mCurrentSpawnAreaId != spawnAreaId)
        {
            mCurrentSpawnAreaId = spawnAreaId;
        }
    }

    Layer * MngrPlayer::GetSpawnAreaLayer( int spawnAreaId)
    {
        return mSpawnAreas.Get(spawnAreaId);
    }


    // continue playback from a stop
    void MngrPlayer::Continue(const ImmCore::piTick now)
    {
        if (mIsWaitingForInput)
        {
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"Continue");
#endif
            if (mIsPaused)
                mIsPaused = false;
            Layer* root = mDocument->GetRoot();
            const piTick rootTime = root->GetStopTime() - root->GetStartTime();
            root->SetStartTime(now - rootTime);
            root->SetStopTime(now - rootTime -1);
            root->SetPlaying(true);
            mIsWaitingForInput = false;
        }
    }


    // go to next play marker (if any play markers) or next stop marker.
    void MngrPlayer::SkipForward(const piTick now)
    {
        if (mNumChapters>0 && mCurrentChapter < mNumChapters - 1)
        {
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"Skip Forward");
#endif
            if (mIsPaused)
                mIsPaused = false;
            Layer* root = mDocument->GetRoot();
            piTick rootTime = root->GetPlaying() ? now - root->GetStartTime() : root->GetStopTime() - root->GetStartTime();
            const int numKeys = root->GetNumAnimKeys(Layer::AnimProperty::Action);
            for (int i = 0; i < numKeys; i++)
            {
                const Layer::AnimKey* key = root->GetAnimKey(Layer::AnimProperty::Action, i);
                const Layer::AnimAction action = static_cast<Layer::AnimAction>(key->mValue.mInt);
                if (key->mTime > rootTime)
                {
                    if (mNumPlayMarkers > 0)
                    {
                        if (action == Layer::AnimAction::Play)
                        {
                            mLog->Printf(LT_MESSAGE, L"Jumping forward to Play index %d, goto time %f  root time %f", i, piTick::ToSeconds(key->mTime),piTick::ToSeconds(rootTime));
                            iJumpAndReset(now, key->mTime);
                            return;
                        }
                    }
                    else //if (mNumStopMarkers > 0)
                    {
                        if (action == Layer::AnimAction::Stop)
                        {
                            iJumpAndReset(now, key->mTime+1);
                            return;
                        }
                    }
                }
            }
        }
    }

    // go to previous play marker (if any play markers) or previous stop marker.
    void MngrPlayer::SkipBack(const piTick now)
    {
        if (mNumChapters > 0 && mCurrentChapter > 0)
        {
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"Skip Back");
#endif
            if (mIsPaused)
                mIsPaused = false;
            Layer* root = mDocument->GetRoot();
            piTick rootTime = root->GetPlaying() ? now - root->GetStartTime() : root->GetStopTime() - root->GetStartTime();
            const int numKeys = root->GetNumAnimKeys(Layer::AnimProperty::Action);
            int numSkip = 0;
            for (int i = 0; i < numKeys; i++)
            {
                const Layer::AnimKey* key = root->GetAnimKey(Layer::AnimProperty::Action, numKeys -i -1);
                const Layer::AnimAction action = static_cast<Layer::AnimAction>(key->mValue.mInt);
                if (key->mTime < rootTime)
                {
                    if (mNumPlayMarkers > 0)
                    {
                        if (action == Layer::AnimAction::Play)
                        {
                            numSkip++;
                            if (numSkip == 2)
                            {
                                mLog->Printf(LT_MESSAGE, L"Jumping back to Play index %d, goto time %f  root time %f", numKeys - i - 1, piTick::ToSeconds(key->mTime), piTick::ToSeconds(rootTime));
                                iJumpAndReset(now, key->mTime);
                                return;
                            }
                        }
                    }
                    else //if (mNumStopMarkers > 0)
                    {
                        if (action == Layer::AnimAction::Stop)
                        {
                            numSkip++;
                            if (numSkip == 2)
                            {
                                iJumpAndReset(now, key->mTime);
                                return;
                            }
                        }
                    }
                }
            }
            if (rootTime > 0)
            {
                mLog->Printf(LT_MESSAGE, L"Jumping back to 0 root time %f", piTick::ToSeconds(rootTime));
                iJumpAndReset(now, piTick(0));
            }
        }
    }

    void MngrPlayer::SetTime(const piTick now, piTick timeSinceStart, piTick timeSinceStop)
    {
        Layer* root = mDocument->GetRoot();
        const bool isPlaying = timeSinceStop == 0;
#ifdef _DEBUG
        mLog->Printf(LT_DEBUG, L"now: %lld, SetTime %lld %lld", now, timeSinceStart,timeSinceStop);
#endif // _DEBUG
        root->ResetActionStatesAt(timeSinceStart);

        if (isPlaying)
        {
            root->SetStopTime(now - timeSinceStart-1);
            root->SetStartTime(now - timeSinceStart);
            root->SetPlaying(true);
        }
        else
        {
            root->SetStartTime(now - timeSinceStart);
            root->SetStopTime(now - timeSinceStop);
            // we set to true so children keys get visited,
            // since we're at a stop, the stop action will fire immediately
            root->SetPlaying(true);
        }
    }

    void MngrPlayer::GetTime(const piTick now, piTick * timeSinceStart, piTick * timeSinceStop)
    {
        Layer* root = mDocument->GetRoot();

        const bool isStopped = !root->GetPlaying();
        *timeSinceStart = isStopped ? (root->GetStopTime() - root->GetStartTime() + 1) : (now - root->GetStartTime());				   // playing
        *timeSinceStop = isStopped ? (now - root->GetStopTime()) : piTick(0);
#ifdef _DEBUG
        mLog->Printf(LT_DEBUG, L"GetTime: %lld %lld", *timeSinceStart, *timeSinceStart);
#endif // DEBUG

    }

    bool MngrPlayer::GetIsWaiting(void) const
    {
        return mIsWaitingForInput;
    }

    bool MngrPlayer::GetIsFinished(void) const
    {
        if (mNumStopMarkers > 0)
        {
            if (mIsWaitingForInput)
            {
                Layer* root = mDocument->GetRoot();
				//  when we set stop time we do "SetStopTime(GetStartTime() + lastKey->mTime + piTick(-1));"

                piTick rootTime = root->GetStopTime() - root->GetStartTime();
                const int numKeys = root->GetNumAnimKeys(Layer::AnimProperty::Action);
                for (int i = 0; i < numKeys; i++)
                {
                    const Layer::AnimKey* key = root->GetAnimKey(Layer::AnimProperty::Action, numKeys-i-1);
                    const Layer::AnimAction action = static_cast<Layer::AnimAction>(key->mValue.mInt);
                    if (action == Layer::AnimAction::Stop)
                    {
                        if (rootTime >= key->mTime)
                        {
                            return true; // at or past last stop
                        }
                        else
                        {
                            return false;
                        }
                    }
                }
            }
        }
        return false;
    }

    size_t MngrPlayer::GetCurrentChapter(void) const
    {
        return mCurrentChapter;
    }

    size_t MngrPlayer::GetChapterCount(void) const
    {
        return mNumChapters;
    }


    void MngrPlayer::Update(const piTick now)
    {
        if (!mIsPaused)
        {
            // Update all layer states
            auto update = [this, &now](Layer* layer, int level, int child, bool instance) -> bool
            {
                layer->SetStateAt(now, mLog);
                return true;
            };
            mDocument->Recurse(update, false, false, false, false);

            Layer* root = mDocument->GetRoot();

            if (root->GetPlaying())
            {
                // If hit loop key, rewind and play.
                piTick rootTime = now - root->GetStartTime();
                if (mHasLoopKey && rootTime >= mLoopKeyTime)
                {
                    Restart(now);
                }

                // If root has stopped, wait for user input
                if (root->GetStopTime() > root->GetStartTime())
                {
    #ifdef _DEBUG
                    mLog->Printf(LT_DEBUG, L"Stop reached, waiting for user input");
    #endif
                    root->SetPlaying(false);
                    mIsWaitingForInput = true;
                }
            }


            // Update current chapter
            if (mNumChapters > 0)
            {
                piTick rootTime = root->GetPlaying() ? now - root->GetStartTime() : root->GetStopTime() - root->GetStartTime();
                const int numKeys = root->GetNumAnimKeys(Layer::AnimProperty::Action);
                mCurrentChapter = 0;
                for (int i = 0; i < numKeys; i++)
                {
                    const Layer::AnimKey* key = root->GetAnimKey(Layer::AnimProperty::Action, i);
                    const Layer::AnimAction action = static_cast<Layer::AnimAction>(key->mValue.mInt);
                    if (key->mTime < rootTime)
                    {
                        if (mNumPlayMarkers > 0)
                        {
                            if (action == Layer::AnimAction::Play)
                            {
                                mCurrentChapter++;
                            }
                        }
                        else //if (mNumStopMarkers > 0)
                        {
                            if (action == Layer::AnimAction::Stop)
                            {
                                mCurrentChapter++;
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }

    void MngrPlayer::iJumpAndReset(const ImmCore::piTick now, const ImmCore::piTick rootTime)
    {
        auto resetLayer = [this, &now](Layer* layer, int level, int child, bool instance) -> bool
        {
            switch (layer->GetType())
            {
            case Layer::Type::Sound:
            {
                LayerSound *ls = (LayerSound*)layer->GetImplementation();
                ls->SetPlaying(false);
                ls->SetPaused(false);
                ls->SetForceRestart(true);
                break;
            }
            case Layer::Type::Paint:
            {
                LayerPaint * lp = (LayerPaint*)layer->GetImplementation();
                lp->SetPlaying(false);
                break;
            }
            case Layer::Type::Group:
                layer->SetPlaying(false);
                layer->SetStopTime(piTick(0));
                break;

            default:
                break;
            }
            layer->SetDefaultPlaybackVisibility();
            return true; // recurse
        };

        mDocument->Recurse(resetLayer, false, false, false, false);

        // Start playback of root
        Layer* root = mDocument->GetRoot();
        root->ResetActionStatesAt(rootTime);
        root->SetVisible(true);
        root->SetPlaying(true);
        root->SetStopTime(now - rootTime - 1);
        root->SetStartTime(now - rootTime);
        mIsWaitingForInput = false;

    }

    int MngrPlayer::FindSpawnAreaIdByLayer(const Layer* spawnAreaLayer) const
    {
        for (int i = 0; i < GetSpawnAreaCount(); ++i)
        {
            if (mSpawnAreas.Get(i) == spawnAreaLayer)
            {
                return i;
            }
        }
        return -1;
    }

}
