//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <string.h>
#include <inttypes.h>

#include "TBE_AudioEngine.h"
#include "TBE_AudioObject.h"



#include "../../libBasics/piTArray.h"
#include "../../libBasics/piPool.h"
#include "../../libBasics/piLog.h"
#include "../../libBasics/piDebug.h"
#include "../../libBasics/piStr.h"
#include "../../libBasics/piVecTypes.h"

#include "piSoundEngineAudioSDKBackend.h"

#include "fakeWAV.h"
#include "memStream.h"

#if defined(ANDROID)

#include <android/log.h>
#include <cmath>
#include <inttypes.h>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "piSoundEngineAudioSDK", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "piSoundEngineAudioSDK", __VA_ARGS__)

#endif

namespace ImmCore {

    using namespace TBE;

    static inline TBE::TBVector pi2tb(const ImmCore::vec3d & v)
    {
        // NOTE: we lose precision! double -> float
        return TBE::TBVector((float)v.x, (float)v.y, (float)v.z);
    }

    static inline TBE::TBQuat pi2tb(const  ImmCore::quatd & q)
    {
        // NOTE: we lose precision! double -> float
        return TBE::TBQuat((float)q.x, (float)q.y, (float)q.z, (float)q.w);
    }

    static inline vec3d tb2pi(const TBE::TBVector v)
    {
        return vec3d((double)v.x, (double)v.y, (double)v.z);
    }

    static inline ImmCore::quatd tb2pi(const TBE::TBQuat & q)
    {
        // NOTE: we lose precision! double -> float
        return ImmCore::quatd((double)q.x, (double)q.y, (double)q.z, (double)q.w);
    }


    class iSoundEngineAudioSDKBackend
    {
    public:
        iSoundEngineAudioSDKBackend() : mSounds(), mEngine(nullptr), mScaleObsolete(1.0) { }
        ~iSoundEngineAudioSDKBackend() {}

        struct Sound
        {
            AudioObject* mAudioObject;
            ChannelMap mChannelMap;
            int32_t mNumChannels;
            piSoundEngine::SoundType  mType;

            struct ModifierParameters
            {
                piSoundEngine::ModifierType mType;
                struct Params// if i make this a union, the constructors have issues (is there a way I can tell the compiler which union member needs to win?)
                {
                    struct DirectionalCone
                    {
                        float mAngleIn;   // in radians, convert to degrees for user input/UI purposes
                        float mAngleBand; // in radians. angleOut will be angleIn = angleBand
                        float mAttenOut;  // value of the attenuation at angleOut
                    }mDirectionalCone;
                    struct DirectionalFrustum
                    {
                        float mAngleInX;
                        float mAngleInY;
                        float mAngleBand; // in radians
                        float mAttenOut;  // value of the attenuation at angleOut
                    }mDirectionalFrus;
                }mParams;
            };
            int   mID;
            FakeWav* mStream;
            MemStream* mCompressedStream;
            piLog* mLog;
            piSoundEngine::PlaybackState mState;
            ModifierParameters mModifier;

            void Init()
            {
                mModifier.mType = piSoundEngine::ModifierType::None;
                mID = -1;
                mState = piSoundEngine::PlaybackState::Stopped;
                mAudioObject = nullptr;
                mChannelMap = ChannelMap::UNKNOWN;
                mNumChannels = 0;
                mStream = nullptr;
                mCompressedStream = nullptr;
                mType = piSoundEngine::SoundType::Num;
            }
        };


        iSoundEngineAudioSDKBackend::Sound* allocNewSound();
        void fixCallbackPointers(uint64_t oldMax);

        AudioEngine* pEngine = nullptr;
        piTArray<piString> mDevices;
        piPool mSounds;
        piSoundEngine *mEngine;
        piLog *mLog;
        double mScaleObsolete;

        void (*mCallbackFunction)(Event, void*); //Function pointer for audio object callback

        //void LogUsage(piLog *log) { const int64_t time = pEngine->getDSPTime(); log->Printf(LT_MESSAGE, L"%d", time); }
    };

    //----------------------------------------------------------------------------------
    /*
    static ChannelMap ChannelMapFromFormat(const int numChannels)
    {
    if (4 == numChannels) return ChannelMap::AMBIX_4;
    else if (10 == numChannels) return ChannelMap::TBE_8_2;
    else if (9 == numChannels) return ChannelMap::AMBIX_9;
    else if (11 == numChannels) return ChannelMap::AMBIX_9_2;
    else if (2 >= numChannels) return ChannelMap::INVALID;
    return ChannelMap::INVALID;
    }

    static bool IsAmbisonic(const ChannelMap channelMap)
    {
    return (ChannelMap::INVALID != channelMap);
    }*/


    //Loop through all the pool addresses to fix the callbacks since after a resize
    //they point to garbage data
    void iSoundEngineAudioSDKBackend::fixCallbackPointers(uint64_t oldMax)
    {
        for (int i = 0; i < oldMax; i++)
        {
            if (mSounds.IsUsed(i))
            {
                iSoundEngineAudioSDKBackend::Sound* me = (iSoundEngineAudioSDKBackend::Sound*)mSounds.GetAddress(i);
                me->mAudioObject->setEventCallback(mCallbackFunction, me);
            }
        }
    }

    iSoundEngineAudioSDKBackend::Sound* iSoundEngineAudioSDKBackend::allocNewSound()
    {

        bool isNew;
        uint64_t id;
        uint64_t oldMax = mSounds.GetMaxLength();
        iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mSounds.Alloc(&isNew, &id, true);

        if (oldMax < mSounds.GetMaxLength()) //A resize happened, so we need to reset all the callbacks with updated pointers
        {
            fixCallbackPointers(oldMax);
        }

        if (me == nullptr)
            return nullptr;

        me->Init();
        me->mID = static_cast<int>(id);

        return me;
    }

    class piSoundEngineAudioSDK : public piSoundEngine
    {
    public:


        static void iSoundObjectCallback(Event ev, void* userData)
        {
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound *)userData;

            switch (ev)
            {
                case Event::END_OF_STREAM:
                    me->mState = PlaybackState::Stopped;
#ifdef _DEBUG
                    me->mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::Event(): id = %d EOS play state", me->mID);
#endif
                    break;
                case Event::PLAY_STATE_CHANGED:
                {
                    switch (me->mAudioObject->getPlayState())
                    {
                    case PlayState::INVALID:
                        me->mLog->Printf(LT_ERROR, L"piSoundEngineAudioSDK::Event(): id = %d INVALID play state", me->mID);
                        break;

                    case PlayState::PAUSED:
                        me->mState = PlaybackState::Paused;
#ifdef _DEBUG
                        me->mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::Event(): id = %d PAUSED play state", me->mID);
#endif
                        break;

                    case PlayState::PLAYING:
                        me->mState = PlaybackState::Playing;
#ifdef _DEBUG
                        me->mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::Event(): id = %d PLAYING play state", me->mID);
#endif
                        break;

                    case PlayState::STOPPED:
                        me->mState = PlaybackState::Stopped;
#ifdef _DEBUG
                        me->mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::Event(): id = %d STOPPED play state", me->mID);
#endif
                        break;

                    }
                    break;
                }

                case Event::ERROR_BUFFER_UNDERRUN: break;

                case Event::DECODER_INIT:
#ifdef _DEBUG
                    me->mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::Event(): id = %d DECODER INIT EVENT", me->mID);
#endif

                    break;

                case Event::ERROR_DECODER_FAIL:
#ifdef _DEBUG
                    me->mLog->Printf(LT_ERROR, L"piSoundEngineAudioSDK::Event(): id = %d DECODER FAIL EVENT", me->mID);
#endif
#ifdef ANDROID
                    LOGD("piSoundEngineAudioSDK::Event(): id = %d DECODER FAIL EVENT", me->mID);
#endif
                    break;
                case Event::ERROR_QUEUE_STARVATION:
#ifdef _DEBUG
                    me->mLog->Printf(LT_ERROR, L"piSoundEngineAudioSDK::Event(): id = %d QUEUE STARVATION EVENT", me->mID);
#endif
#ifdef ANDROID
                    LOGD("piSoundEngineAudioSDK::Event(): id = %d QUEUE STARVATION EVENT", me->mID);
#endif
                    break;
                case Event::INVALID:
#ifdef _DEBUG
                    me->mLog->Printf(LT_ERROR, L"piSoundEngineAudioSDK::Event(): id = %d INVALID EVENT", me->mID);
#endif
#ifdef ANDROID
                    LOGD("piSoundEngAudioSDK::Event(): id = %d INVALID EVENT", me->mID);
#endif

                    break;
                case Event::LOOPED:
#ifdef _DEBUG
                    me->mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::Event(): id = %d LOOP EVENT", me->mID);
#endif
                    break;

            }

        }


        static inline bool ConfigureSoundChannels(iSoundEngineAudioSDKBackend::Sound* me, bool makePositional)
        {
            if (me->mNumChannels == 4)
            {
                me->mType = SoundType::Ambisonic;
                me->mChannelMap = ChannelMap::AMBIX_4;
            }
            else if (me->mNumChannels == 9)
            {
                me->mType = SoundType::Ambisonic;
                me->mChannelMap = ChannelMap::AMBIX_9;
            }
            else if (me->mNumChannels == 10)
            {
                me->mType = SoundType::Ambisonic;
                me->mChannelMap = ChannelMap::TBE_8_2;
            }
            else if (me->mNumChannels == 11)
            {
                me->mType = SoundType::Ambisonic;
                me->mChannelMap = ChannelMap::AMBIX_9_2;
            }
            else if (me->mNumChannels == 2)
            {
                if (makePositional)
                {
                    me->mType = SoundType::Positional;
                    me->mChannelMap = ChannelMap::HEADLOCKED_CHANNEL0;
                }
                else
                {
                    me->mType = SoundType::Flat;
                    me->mChannelMap = ChannelMap::STEREO;
                }
            }
            else if (me->mNumChannels == 1)
            {
                if (makePositional)
                {
                    me->mType = SoundType::Positional;
                    me->mChannelMap = ChannelMap::MONO;
                }
                else
                {
                    me->mType = SoundType::Flat;
                    me->mChannelMap = ChannelMap::MONO;
                }
            }
            else
                return false;

            return true;
        }

        /*
            Add from callback
        */
        int AddSound(void(*callback)(float* channelBuffer, size_t numSamples, size_t numChannels, void* userData), void * userData)
        {
            iSoundEngineAudioSDKBackend::Sound *me = mBk->allocNewSound();
            if (me == nullptr)
                return -1;

            AudioObject* pAudioObject = nullptr;
            EngineError er = mBk->pEngine->createAudioObject(pAudioObject);
            if (er != EngineError::OK)
            {
                mBk->mSounds.Free(me->mID);
                return -1;
            }

            me->mNumChannels = 1;
            me->mType = SoundType::Positional;
            me->mChannelMap = ChannelMap::MONO;
            me->mLog = mLog;
            me->mAudioObject = pAudioObject;

            pAudioObject->shouldSpatialise(true);
            pAudioObject->setAudioBufferCallback(callback, me->mNumChannels, me->mChannelMap, userData);
            pAudioObject->play();
            return me->mID;
        }

        /*
            Add from file
        */
        int AddSound(const wchar_t* filenameW, bool makePositional)
        {
            piAssert(nullptr != filenameW);

            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->allocNewSound();

            if (me == nullptr)
                return -1;

            char* filenameA = piws2str(filenameW);

            // Get the number of channels from the file
            AudioFormatDecoder* pDecoder = nullptr;
            if (TBE_CreateAudioFormatDecoder(pDecoder, filenameA, 1024, 0.0f) != EngineError::OK)
            {
                mBk->mSounds.Free(me->mID);
                return -1;
            }
            me->mNumChannels = pDecoder->getNumOfChannels();
            delete pDecoder;

            if (!ConfigureSoundChannels(me, makePositional))
            {
                mBk->mSounds.Free(me->mID);
                return -1;
            }

            AudioObject* pAudioObject = nullptr;
            EngineError er = mBk->pEngine->createAudioObject(pAudioObject);
            if (er != EngineError::OK)
            {
                mBk->mSounds.Free(me->mID);
                return -1;
            }

            if (pAudioObject->open(filenameA) != EngineError::OK)
            {
                mBk->mSounds.Free(me->mID);
                return -1;
            }

            pAudioObject->shouldSpatialise(makePositional);

            me->mAudioObject = pAudioObject;
            me->mLog = mLog;
            me->mState = PlaybackState::Stopped;
            me->mAudioObject->setEventCallback(iSoundObjectCallback, me);
            free(filenameA);
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::AddSound(): filenameW = %s, id = %d", filenameW, me->mID);
#endif
            return me->mID;
        }

        /*
            Compressed OGG Opus stream
        */
        int AddSound(int numChannels, uint64_t length, const void *buffer, bool makePositional)
        {
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->allocNewSound();

            if (me == nullptr)
                return -1;

            me->mNumChannels = numChannels;

            if (!ConfigureSoundChannels(me, makePositional))
            {
                mBk->mSounds.Free(me->mID);
                return -2;
            }

            AudioObject* pAudioObject = nullptr;
            if (mBk->pEngine->createAudioObject(pAudioObject) != EngineError::OK)
            {
                mBk->mSounds.Free(me->mID);
                return -3;
            }

            me->mCompressedStream = new MemStream(length, buffer);
            TBE::IOStream *streams = me->mCompressedStream;

            if (pAudioObject->open(streams, true) != EngineError::OK)
            {
                mBk->mSounds.Free(me->mID);
                return -4;
            }

            pAudioObject->shouldSpatialise(me->mType == SoundType::Positional);

            me->mAudioObject = pAudioObject;
            me->mState = PlaybackState::Stopped;
            me->mLog = mLog;
            me->mAudioObject->setEventCallback(iSoundObjectCallback, me);
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::AddSound(): id = %d", me->mID);
#endif
            return me->mID;
        }


        /*
            Uncompressed PCM wav
        */
        int AddSound(const int frequency, int precision, int numChannels, uint64_t length, const void *buffer, bool makePositional)
        {
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->allocNewSound();

            if (me == nullptr)
                return -1;

            me->mNumChannels = numChannels;

            if (!ConfigureSoundChannels(me, makePositional))
            {
                mBk->mSounds.Free(me->mID);
                return -2;
            }

            AudioObject* pAudioObject = nullptr;
            if (mBk->pEngine->createAudioObject(pAudioObject) != EngineError::OK)
            {
                mBk->mSounds.Free(me->mID);
                return -3;
            }

            me->mStream = new FakeWav(frequency, precision, numChannels, length, buffer);
            TBE::IOStream *streams = me->mStream;
            if (pAudioObject->open(streams, true) != EngineError::OK)
            {
                mBk->mSounds.Free(me->mID);
                return -4;
            }

            pAudioObject->shouldSpatialise(me->mType == SoundType::Positional);

            me->mAudioObject = pAudioObject;
            me->mState = PlaybackState::Stopped;
            me->mLog = mLog;
            me->mAudioObject->setEventCallback(iSoundObjectCallback, me);
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::AddSound(): id = %d", me->mID);
#endif
            return me->mID;
        }

        void DelSound(int id)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::DelSound(): id = %d", id);
#endif
            if (me->mStream) me->mStream->close();
            if (me->mCompressedStream) me->mCompressedStream->close();
            if (me->mAudioObject->isOpen()) me->mAudioObject->close();
            mBk->pEngine->destroyAudioObject(me->mAudioObject);
            mBk->mSounds.Free(id);
            me->mID = -1;
        }

        bool Play(int id, uint64_t microSecondOffset)
        {
            piAssert(id >= 0 && id < mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));

            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            if (me->mAudioObject->seekToMs(float(double(microSecondOffset) / 1000.)) != EngineError::OK)
                mLog->Printf(LT_ERROR, L"piSoundEngineAudioSDK::Play(): id = %d, offset %llu SEEK FAILED!", id, microSecondOffset);

#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::Play(): id = %d, offset %llu", id, microSecondOffset);
#endif
            if (me->mState == PlaybackState::Playing || me->mState == PlaybackState::PlayingStarting)
            {
#ifdef _DEBUG
                mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::Play(): id = %d interrupted wrong state",id);
#endif
                return false;
            }
            if (me->mAudioObject->play() != EngineError::OK)
                return false;

            me->mState = PlaybackState::PlayingStarting;

            return true;
        }



        bool Stop(int id)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::Stop(): id = %d", id);
#endif
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);

            if (me->mState == PlaybackState::Stopped || me->mState == PlaybackState::StoppingStarted) return false;

            if (me->mAudioObject->stop() != EngineError::OK)
                return false;

            // with FBA we must reset the sound here so we can restart it with an arbitrary seek later
            me->mAudioObject->seekToMs(0.0f);

            me->mState = PlaybackState::StoppingStarted;

            return true;
        }

        bool Pause(int id)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::Pause(): id = %d", id);
#endif
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);

            if (me->mAudioObject == nullptr) return false;
            if (me->mState != PlaybackState::Playing || me->mState == PlaybackState::PlayingStarting) return false;

            me->mState = PlaybackState::PausingStarted;

            if (me->mAudioObject->pause() != EngineError::OK)
                return false;

            return true;
        }

        bool Resume(int id)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG, L"piSoundEngineAudioSDK::Resume(): id = %d", id);
#endif
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);

            if (me->mState != PlaybackState::Paused) return false;
            if (me->mAudioObject == nullptr) return false;

            if (me->mAudioObject->play() != EngineError::OK)
                return false;

            return true;
        }

        void PauseAllSounds(void)
        {
            const int num = static_cast<int>(mBk->mSounds.GetMaxLength());
            for (int i = 0; i < num; i++)
            {
                if(mBk->mSounds.IsUsed(i))
                    this->Pause(i);
            }
        }

        void ResumeAllSounds(void)
        {
            const int num = static_cast<int>(mBk->mSounds.GetMaxLength());
            for (int i = 0; i < num; i++)
            {
                if (mBk->mSounds.IsUsed(i))
                    this->Resume(i);
            }
        }

        piSoundEngine::PlaybackState GetPlaybackState(int id)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            const iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            return me->mState;
        }

        float GetVolume(int id) const
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            return me->mAudioObject->getVolume();
        }

        bool SetVolume(int id, float volume)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            me->mAudioObject->setVolume(volume, 0.0f, true);
            return true;
        }

        SoundType GetType(int id) const
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);

#ifdef _DEBUG
            if (me->mType == SoundType::Positional)
                piAssert(me->mAudioObject->isSpatialised());
#endif

            return me->mType;
        }

        bool ConvertType(int id, SoundType dtype)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            SoundType stype = me->mType;

            if (stype == SoundType::Flat && dtype == SoundType::Flat)
            {
                return true;
            }
            else if (stype == SoundType::Flat && dtype == SoundType::Positional)
            {
                me->mAudioObject->shouldSpatialise(true);
                me->mType = SoundType::Positional;
                return true;
            }
            else if (stype == SoundType::Positional && dtype == SoundType::Positional)
            {
                return true;
            }
            else if (stype == SoundType::Positional && dtype == SoundType::Flat)
            {
                me->mAudioObject->shouldSpatialise(false);
                me->mType = SoundType::Flat;
                return true;
            }

            piAssert(false);
            return false;
        }

        bool CanConvertType(int id, SoundType dtype) const
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            SoundType stype = me->mType;

            if (dtype == SoundType::Flat)
            {
                if (stype == SoundType::Flat)       return true;
                if (stype == SoundType::Positional) return true;
                if (stype == SoundType::Ambisonic)  return false;
            }
            else if (dtype == SoundType::Positional)
            {
                if (stype == SoundType::Flat)       return me->mNumChannels == 1;
                if (stype == SoundType::Positional) return true;
                if (stype == SoundType::Ambisonic)  return false;
            }
            else if (dtype == SoundType::Ambisonic)
            {
                if (stype == SoundType::Flat)       return false;
                if (stype == SoundType::Positional) return false;
                if (stype == SoundType::Ambisonic)  return true;
            }

            return false;
        }

        bool GetLooping(int id) const
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            const iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            return me->mAudioObject->loopingEnabled();
            }

        void SetLooping(int id, bool loop)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            me->mAudioObject->enableLooping(loop);
#ifdef _DEBUG
            mLog->Printf(LT_DEBUG,L"Setting sound %d to looping %d",id, loop);
#endif
            }

        bool GetPaused(int id) const
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            const iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            const PlayState playState = me->mAudioObject->getPlayState();
            return (PlayState::PAUSED == playState);
        }

        AttenuationType GetAttenMode(int id) const
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            const iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            piAssert(me->mType != SoundType::Ambisonic);

            AttenuationMode attenuationMode = me->mAudioObject->getAttenuationMode();
            if (attenuationMode == AttenuationMode::LINEAR) return AttenuationType::Linear;
            if (attenuationMode == AttenuationMode::LOGARITHMIC) return AttenuationType::Logarithmic;
            else return AttenuationType::None;
        }

        void SetAttenMode(int id, AttenuationType attenMode)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            const iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            piAssert(me->mType != SoundType::Ambisonic);
            piAssert(attenMode != AttenuationType::InverseSquare);


            AttenuationMode attenuationMode = AttenuationMode::CUSTOM;
            if (attenMode == AttenuationType::Logarithmic)  attenuationMode = AttenuationMode::LOGARITHMIC;
            if (attenMode == AttenuationType::Linear)       attenuationMode = AttenuationMode::LINEAR;

            me->mAudioObject->setAttenuationMode(attenuationMode);
        }

        void SetAttenMinMax(int id, double attenMin, double attenMax)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            const iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            piAssert(me->mType == SoundType::Positional);
            AttenuationProps attenProps;
            attenProps.minimumDistance = float(attenMin*mBk->mScaleObsolete);
            attenProps.maximumDistance = float(attenMax*mBk->mScaleObsolete);
            // FB audio factor 1 does 6db of attenuation per doubling of distance
            const float minToMax = log2f(attenProps.maximumDistance / (attenProps.minimumDistance+0.001f) );
            attenProps.factor = 5.0f / minToMax; // this seems to match pretty well
            attenProps.maxDistanceMute = true;
            me->mAudioObject->setAttenuationProperties(attenProps);
        }

        void SetModifierType(int id, ModifierType modifType)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            piAssert(me->mType != SoundType::Ambisonic);
            me->mModifier.mType = modifType;
        }
        void SetModifierCone(int id, double angleInner, double angleBand, double attenOut)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            piAssert(me->mType == SoundType::Positional);
            piAssert(me->mModifier.mType == ModifierType::Cone);
            me->mModifier.mParams.mDirectionalCone.mAngleIn = float(angleInner);
            me->mModifier.mParams.mDirectionalCone.mAngleBand = float(angleBand);
            me->mModifier.mParams.mDirectionalCone.mAttenOut = float(attenOut);
        }
        void SetModifierFrustum(int id, double angleInnerX, double angleInnerY, double angleBand, double attenOut)
        {
            piAssert(id >= 0 && id < mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            piAssert(me->mType == SoundType::Positional);
            piAssert(me->mModifier.mType == ModifierType::Frus);
            me->mModifier.mParams.mDirectionalFrus.mAngleInX = float(angleInnerX);
            me->mModifier.mParams.mDirectionalFrus.mAngleInY = float(angleInnerY);
            me->mModifier.mParams.mDirectionalFrus.mAngleBand = float(angleBand);
            me->mModifier.mParams.mDirectionalFrus.mAttenOut = float(attenOut);
        }
        float ComputeModifiers(int id)
        {
            piAssert(id >= 0 && id < mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            const iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);

            if (me->mType == SoundType::Positional)
            {
                if (me->mModifier.mType == ModifierType::Cone)
                {
                    const iSoundEngineAudioSDKBackend::Sound::ModifierParameters::Params::DirectionalCone *params = &me->mModifier.mParams.mDirectionalCone;
                    // sound source transformation
                    const vec3d pos = tb2pi(me->mAudioObject->getPosition());
                    const quatd rot = tb2pi(me->mAudioObject->getRotation());
                    const trans3d soundToWorld = trans3d(rot, 1.0, flip3::N, pos);
                    // listener in world space
                    const vec3d wpos = tb2pi(mBk->pEngine->getListenerPosition());
                    // convert listener to sound source space
                    const vec3d spos = (invert(soundToWorld) * vec4d(wpos, 1.0)).xyz();
                    // dot the listener with the foward vector z=(0,0,-1) // we use negative because TBE is left handed (DX), not right (GL)
                    const float ff = clamp01(float(-normalize(spos).z));

                    // do attenuation math
                    #if 0
                    // cosine space
                    const float fa = cosf(float(params->mAngleIn));
                    const float fb = cosf(float(params->mAngleIn + params->mAngleBand));
                    //piAssert(fa > fb);
                    return smoothstep(fb, fa, ff);
                    #else
                    // angular space - feels much more linear as we get inside the cone
                    const float fa = float(params->mAngleIn);
                    const float fb = float(params->mAngleIn + params->mAngleBand);
                    float atten = 1.0f-smoothstep(fa, fb, acosf(ff));
                    return clamp01(atten) * (1.0f - params->mAttenOut) + params->mAttenOut;
                    #endif
                }
                else if (me->mModifier.mType == ModifierType::Frus)
                {
                    const iSoundEngineAudioSDKBackend::Sound::ModifierParameters::Params::DirectionalFrustum *params = &me->mModifier.mParams.mDirectionalFrus;
                    // sound source transformation
                    const vec3d pos = tb2pi(me->mAudioObject->getPosition());
                    const quatd rot = tb2pi(me->mAudioObject->getRotation());
                    const trans3d soundToWorld = trans3d(rot, 1.0, flip3::N, pos);
                    // listener in world space
                    const vec3d wpos = tb2pi(mBk->pEngine->getListenerPosition());
                    // convert listener to sound source space
                    const vec3d dpos = (invert(soundToWorld) * vec4d(wpos, 1.0)).xyz();
                    // convert from DX to GL
                    const vec3d spos = dpos*vec3d(1.0f,1.0f,-1.0f);
                    // if behind, no sound
                    if( spos.z<0.0f ) return 0.0f;
                    // project listener into sound Z=1 plane
                    const vec2 p = d2f(spos.xy()/spos.z); // it's ok to convert to float here, we already projected and made all coordinates local

                    const vec2 innerBox = vec2(tanf(params->mAngleInX), tanf(params->mAngleInY));
                    const vec2 outerBox = vec2(tanf(params->mAngleInX + params->mAngleBand), tanf(params->mAngleInY + params->mAngleBand));
                    const vec2 dInner = abs(p) - innerBox;
                    const vec2 vAtt = vec2(smoothstep(0.0f, outerBox.x-innerBox.x, dInner.x), smoothstep(0.0f, outerBox.y-innerBox.y, dInner.y));
                    const float att = 1.0f - std::max(vAtt.x,vAtt.y);

                    // square power, for more perceptual gradient
                    return clamp01(att * att) * (1.0f - params->mAttenOut) + params->mAttenOut;
                }
            }
            return 1.0f;
        }

        void SetListener( const trans3d & viewerToWorld)
        {
            const double *pos = (const double*)&viewerToWorld.mTranslation;
            const double *rot = (const double*)&viewerToWorld.mRotation;
            const double sca = viewerToWorld.mScale;

            mBk->pEngine->setListenerPosition(TBE::TBVector(float(pos[0]), float(pos[1]), float(pos[2])));
            mBk->pEngine->setListenerRotation(TBE::TBQuat(float(rot[0]), float(rot[1]), float(rot[2]), float(rot[3])));
            mBk->mScaleObsolete = sca;
        }

        void SetPosition(int id, const double* pos)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));

            const iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            piAssert(me->mType == SoundType::Positional);
            me->mAudioObject->setPosition(pi2tb(vec3d(pos)));
        }

        void SetOrientation(int id, const double* dir, const double* up)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            const iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            piAssert(me->mType == SoundType::Positional || me->mType == SoundType::Ambisonic);
            me->mAudioObject->setRotation(pi2tb(vec3d(dir)), pi2tb(vec3d(up)));
        }

        void SetPositionOrientation(int id, const double* pos, const double* dir, const double* up)
        {
            piAssert(id >= 0 && id<mBk->mSounds.GetMaxLength() && mBk->mSounds.IsUsed(id));
            const iSoundEngineAudioSDKBackend::Sound *me = (iSoundEngineAudioSDKBackend::Sound*)mBk->mSounds.GetAddress(id);
            piAssert(me->mType == SoundType::Positional || me->mType == SoundType::Ambisonic);
            me->mAudioObject->setPosition(pi2tb(vec3d(pos)));
            me->mAudioObject->setRotation(pi2tb(vec3d(dir)), pi2tb(vec3d(up)));
        }

    public:
        piSoundEngineAudioSDK(iSoundEngineAudioSDKBackend *bk, piLog *log) { mBk = bk; mLog = log; mBk->mCallbackFunction = &iSoundObjectCallback; }
    private:
        iSoundEngineAudioSDKBackend * mBk;
        piLog *mLog;
    };


    //----------------------------------------------------------------------------------

    piSoundEngineAudioSDKBackend::piSoundEngineAudioSDKBackend() : piSoundEngineBackend()
    {
    }

    piSoundEngineAudioSDKBackend::~piSoundEngineAudioSDKBackend(void)
    {
    };

    bool piSoundEngineAudioSDKBackend::doInit(piLog *log)
    {
        iSoundEngineAudioSDKBackend * me = new iSoundEngineAudioSDKBackend();
        if (!me) return false;
        mData = me;
        me->mEngine = new piSoundEngineAudioSDK(me, log);
        me->mLog = log;
        me->mScaleObsolete = 1.0;

        if (!me->mDevices.Init(1024, false))
            return false;

        const int num = AudioEngine::getNumAudioDevices();
        for (int i = 0; i<num; i++)
        {
            const char *deviceName = AudioEngine::getAudioDeviceName(i);
            if (deviceName == nullptr) continue;
            if (deviceName[0] == '\0') continue;

            piString *dev = me->mDevices.New(1, true);
            if (dev == nullptr) return false;
            if (!dev->InitCopyS(deviceName)) return false;
        }

        if (!me->mSounds.Init(256, sizeof(iSoundEngineAudioSDKBackend::Sound)))
            return false;

        return true;
    }

    void piSoundEngineAudioSDKBackend::doDeinit(void)
    {
        iSoundEngineAudioSDKBackend * me = (iSoundEngineAudioSDKBackend*)mData;
        me->mSounds.End();
        me->mDevices.End();
        delete me->mEngine;
        delete me;
    }


    piSoundEngine* piSoundEngineAudioSDKBackend::GetEngine(void)
    {
        iSoundEngineAudioSDKBackend * me = (iSoundEngineAudioSDKBackend*)mData;
        return me->mEngine;
    }

    int piSoundEngineAudioSDKBackend::GetNumDevices(void)
    {
        iSoundEngineAudioSDKBackend * me = (iSoundEngineAudioSDKBackend*)mData;
        return static_cast<int>(me->mDevices.GetLength());
    }

    const wchar_t* piSoundEngineAudioSDKBackend::GetDeviceName(int id) const
    {
        iSoundEngineAudioSDKBackend * me = (iSoundEngineAudioSDKBackend*)mData;
        return me->mDevices[id].GetS();
    }

    int piSoundEngineAudioSDKBackend::GetDeviceFromName(const wchar_t* name)
    {
        iSoundEngineAudioSDKBackend * me = (iSoundEngineAudioSDKBackend*)mData;

        const uint64_t numDevices = me->mDevices.GetLength();
        for (uint64_t i = 0; i < numDevices; ++i)
        {
            if (me->mDevices[i].EqualW(name))
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

#ifdef WINDOWS
#include <windows.h>
    int piSoundEngineAudioSDKBackend::GetDeviceFromGUID(void* deviceGUID)
    {
        iSoundEngineAudioSDKBackend * me = (iSoundEngineAudioSDKBackend*)mData;

        GUID *guid = (GUID*)deviceGUID;
        wchar_t tmp[256];
        piwsprintf(tmp, 256, L"%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
            guid->Data1, guid->Data2, guid->Data3,
            guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
            guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);

        const char* deviceNameA = AudioEngine::getAudioDeviceNameFromId(tmp);
        const wchar_t *deviceNameW = pistr2ws(deviceNameA);
        const uint64_t numDevices = me->mDevices.GetLength();
        for (uint64_t i = 0; i < numDevices; ++i)
        {
            if (me->mDevices[i].EqualW(deviceNameW))
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
#else
    int piSoundEngineAudioSDKBackend::GetDeviceFromGUID(void* deviceGUID)
    {
        return -1;
    }
#endif

    static void iEngineCallback(Event ev, void* userData)
    {
        switch (ev)
        {

        case Event::ERROR_BUFFER_UNDERRUN:
#ifdef _DEBUG
            ((piLog*)userData)->Printf(LT_ERROR, L"piSoundEngineAudioSDK::Event(): BUFFER UNDERRUN EVENT");
#endif
#ifdef ANDROID
            LOGD("piSoundEngineAudioSDK::Event(): BUFFER UNDERRUN EVENT");
#endif
            break;

        default: break;
        }
    }

    bool piSoundEngineAudioSDKBackend::Init(void * hwnd, int deviceID, const piSoundEngineBackend::Configuration* config)
    {
        iSoundEngineAudioSDKBackend * me = (iSoundEngineAudioSDKBackend*)mData;

        EngineInitSettings settings{};
        settings.audioSettings.customAudioDeviceName = (deviceID == -1) ? "" : piws2str(me->mDevices[deviceID].GetS());
        settings.audioSettings.deviceType = (deviceID == -1) ? AudioDeviceType::DEFAULT : AudioDeviceType::CUSTOM;
        settings.audioSettings.sampleRate = float(config->mSampleRate);
        settings.audioSettings.bufferSize = (config->mBufferSize >= 480 ) ? config->mBufferSize : 960;
        settings.platformSettings.useAndroidFastPath = config->mLowLatency;
        settings.experimental.useFba = true;
        settings.experimental.fbaNumThreads = 0;
        settings.memorySettings.spatQueueSizePerChannel = 4096;
        settings.memorySettings.audioObjectPoolSize = config->mMaxSoundVoices > 0 ? config->mMaxSoundVoices : 128;

        //Disable the audio device if the flag is set, since only the offline renderer needs to be able to disable the audio device
        //It's easier to maintain if we set a flag beforehand rather than passing it as an argument to this function
        if (mDisableDevice)
        {
            settings.audioSettings.deviceType = AudioDeviceType::DISABLED;
#ifdef _DEBUG
            me->mLog->Printf(LT_MESSAGE, L"Disabled Audio Device.");
#endif
        }

        EngineError tbeError = TBE_CreateAudioEngine(me->pEngine, settings);
        if (tbeError != TBE::EngineError::OK)
        {
#ifdef _DEBUG
            me->mLog->Printf(LT_ERROR, L"TBE_CreateAudioEngine failed with %d", tbeError);
#endif
            return false;
        }

        if (!mDisableDevice)
        {
          tbeError = me->pEngine->start();
          if (tbeError != TBE::EngineError::OK)
          {
  #ifdef _DEBUG
              me->mLog->Printf(LT_ERROR, L"AudioEngine::Start failed with %d", tbeError);
  #endif
              return false;
          }
        }
        if (me->pEngine->setEventCallback(iEngineCallback, me->mLog) != EngineError::OK)
        {
#ifdef _DEBUG
            me->mLog->Printf(LT_ERROR, L"Couldn't set event callback.");
#endif
            return false;
        }

        return true;
    }

    void piSoundEngineAudioSDKBackend::Deinit(void)
    {
        iSoundEngineAudioSDKBackend * me = (iSoundEngineAudioSDKBackend*)mData;

        const int num = static_cast<int>(me->mSounds.GetMaxLength());
        for (int i = 0; i<num; i++)
        {
            if (!me->mSounds.IsUsed(i)) continue;
            iSoundEngineAudioSDKBackend::Sound *sound = (iSoundEngineAudioSDKBackend::Sound*)me->mSounds.GetAddress(i);
            if (sound->mID != -1)
            {
#ifdef _DEBUG
                me->mLog->Printf(LT_DEBUG, L"Sound leak, id = %d", i);
#endif
            }
        }

        TBE_DestroyAudioEngine(me->pEngine);
    }

    void piSoundEngineAudioSDKBackend::Tick(void)
    {
        iSoundEngineAudioSDKBackend * me = (iSoundEngineAudioSDKBackend*)mData;
        me->pEngine->update();
        //me->LogUsage(me->mLog);
    }

    piSoundEngineBackend* piSoundEngineAudioSDKBackend::Create(piLog * log)
    {
        piSoundEngineAudioSDKBackend* me = new piSoundEngineAudioSDKBackend();

        if (!me->doInit(log))
            return nullptr;

        return me;
    }

    void piSoundEngineAudioSDKBackend::Destroy(piSoundEngineBackend* vme)
    {
        piSoundEngineAudioSDKBackend* me = (piSoundEngineAudioSDKBackend*)vme;

        me->doDeinit();

        delete me;
    }

    //Sets a flag to disable the audio device on Engine Init
    void piSoundEngineAudioSDKBackend::setDisableAudioDevice(piSoundEngineBackend* vme, bool disableDevice)
    {
      piSoundEngineAudioSDKBackend* me = (piSoundEngineAudioSDKBackend*)vme;
      me->mDisableDevice = disableDevice;
    }

    //Gets the sample data from the audio engine.
    //The audio device needs to be disabled for this to work properly.
    //Buffer size should be the number of samples per channel * 2;
    void piSoundEngineAudioSDKBackend::getAudioMix(piSoundEngineBackend* vme, float* buffer, int sampleNumPerChannel)
    {
      piSoundEngineAudioSDKBackend* me = (piSoundEngineAudioSDKBackend*)vme;
      iSoundEngineAudioSDKBackend* engine = (iSoundEngineAudioSDKBackend*)me->mData;

      EngineError err = engine->pEngine->getAudioMix(buffer, sampleNumPerChannel * 2, 2);
      if (err != EngineError::OK) {
        engine->mLog->Printf(LT_ERROR, L"Could not get audio mix, error: %d", err);
      }
    }


}
