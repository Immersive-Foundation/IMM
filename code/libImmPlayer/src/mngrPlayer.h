#pragma once


#include "../../libImmImporter/src/document/layer.h"
#include "../../libImmImporter/src/document/sequence.h"

using namespace ImmImporter;

namespace ImmPlayer
{

    class LayerRendererSound;

    class MngrPlayer
    {

    public:
        MngrPlayer();
        ~MngrPlayer();

        bool Init(ImmCore::piLog* log);
        void Deinit();
        void Update(const ImmCore::piTick now); // should be called every frame

        // All file types
        void Enter(Sequence * document, const ImmCore::piTick now);
        void Exit();
        void Pause(const ImmCore::piTick now);
        void Resume(const ImmCore::piTick now);

        int  GetSpawnAreaCount() const;
        int  GetSpawnArea();
        int  GetInitialSpawnArea();
        void SetSpawnArea(int spawnAreaId);
        Layer * GetSpawnAreaLayer(int spawnAreaId);

        // Stories/Comics
        size_t GetCurrentChapter(void) const;
        size_t GetChapterCount(void) const;
		bool GetHasPlays(void) const;
        bool GetIsWaiting(void) const;  // at a stop
        bool GetIsFinished(void) const; // at the last stop

        void Restart(const ImmCore::piTick now);
        void SkipForward(const ImmCore::piTick now);
        void SkipBack(const ImmCore::piTick now);
        void Continue(const ImmCore::piTick now);

        void SetTime(const ImmCore::piTick now, ImmCore::piTick timeSinceStart, ImmCore::piTick timeSinceStop);
        void GetTime(const ImmCore::piTick now, ImmCore::piTick* timeSinceStart, ImmCore::piTick* timeSinceStop);

        // an animation is paused
        inline bool GetIsPaused(void) const { return mIsPaused; }

        // this tells you is you're between Enter and Exit
        inline bool GetIsPlaying(void) const { return mIsPlaying; }

    private:
        void iJumpAndReset(const ImmCore::piTick now, const ImmCore::piTick rootTime);
        int FindSpawnAreaIdByLayer(const Layer* spawnAreaLayer) const ;

        // References
        Sequence* mDocument;
        ImmCore::piLog* mLog;
        int mCurrentSpawnAreaId;

        size_t mNumPlayMarkers;
        size_t mNumStopMarkers;
        size_t mCurrentChapter;
        size_t mNumChapters;
        bool mHasLoopKey;
        ImmCore::piTick mLoopKeyTime; // init with end of root
        // Internal state
        bool mIsPaused;						// user selected pause
        bool mIsPlaying;					// the player is active (vs authoring mode)
        bool mIsAtEnd;
        bool mIsWaitingForInput;			// story is paused and waiting
        ImmCore::piTArray<Layer*> mSpawnAreas;
    };
}
