#include "libImmCore/src/libBasics/piArray.h"
#include "libImmCore/src/libBasics/piString.h"
#include "libImmCore/src/libBasics/piImage.h"
#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libRender/piRenderer.h"
#include "libImmCore/src/libSound/piSound.h"
#include "libImmCore/src/libBasics/piFile.h"
#include "libImmCore/src/libBasics/piDebug.h"
#include "libImmCore/src/libBasics/piStr.h"
#include "libImmCore/src/libWave/formats/piWaveWAV.h"

#include "libImmImporter/src/document/layerSound.h"

#include "layerRendererSound.h"

using namespace ImmCore;
using namespace ImmImporter;
namespace ImmPlayer
{

	struct iSound
	{
		int   mSoundID = -1;
      //bool  mLoopingCache; // cannot be animated
        bool  mPlayingCached;
        bool  mPausedCached;
        float mFinalVolumeCached;
        uint64_t mOffsetCached;
        LayerSound::AttenuationType mAttenuationTypeCached;
        double mAttenuationMinCached;
        double mAttenuationMaxCached;

        LayerSound::ModifierType mModifierTypeCached;

        double mModifierConeAngleInner;
        double mModifierConeAngleBand;
        double mModifierConeAttenOut;

        double mModifierFrustumAngleInnerX;
        double mModifierFrustumAngleInnerY;
        double mModifierFrustumAngleBand;
        double mModifierFrustumAttenOut;

        //LayerSound::Type mTypeCache;

	};

	LayerRendererSound::LayerRendererSound() : LayerRenderer() {}
	LayerRendererSound::~LayerRendererSound() {}

	bool LayerRendererSound::Init(piRenderer* renderer, piLog* log, Drawing::ColorSpace colorSpace, bool frontIsCCW)
	{
		if (!mSounds.Init(128, sizeof(iSound), false))
			return false;

		return true;
	}
    
	void LayerRendererSound::Deinit(piRenderer* renderer, piLog* log)
	{
		mSounds.End();
	}

	bool LayerRendererSound::LoadInCPU(piLog* log, Layer* la)
	{     
        // We create the sound in the renderer, but not yet in the soundengine
        if (!la->GetLoaded()) return true;

        LayerSound* lp = (LayerSound*)la->GetImplementation();
        const int id = static_cast<int>(mSounds.GetLength());
        lp->SetGpuId(id);

        iSound* snd = (iSound*)mSounds.Alloc(1, true);
        if (!snd)
            return false;

        snd->mSoundID = -1;
        
        snd->mPlayingCached = false;
        snd->mFinalVolumeCached = lp->GetVolume() * lp->GetGain();
        snd->mPausedCached = false;
        snd->mOffsetCached = lp->GetOffset();

        LayerSound::AttenuationParameters atten;
        lp->GetAttenuation(&atten);

        LayerSound::ModifierParameters modif;
        lp->GetModifier(&modif);

        snd->mAttenuationTypeCached = atten.mType;
        snd->mAttenuationMinCached = double(atten.mMin);
        snd->mAttenuationMaxCached = double(atten.mMax);

        snd->mModifierTypeCached = modif.mType;
        snd->mModifierConeAngleInner = double(modif.mParams.mDirectionalCone.mAngleIn);
        snd->mModifierConeAngleBand = double(modif.mParams.mDirectionalCone.mAngleBand);
        snd->mModifierConeAttenOut = double(modif.mParams.mDirectionalCone.mAttenOut);

        snd->mModifierFrustumAngleInnerX = double(modif.mParams.mDirectionalFrus.mAngleInX);
        snd->mModifierFrustumAngleInnerY = double(modif.mParams.mDirectionalFrus.mAngleInY);
        snd->mModifierFrustumAngleBand = double(modif.mParams.mDirectionalFrus.mAngleBand);
        snd->mModifierFrustumAttenOut = double(modif.mParams.mDirectionalFrus.mAttenOut);

		return true;
	}

	void LayerRendererSound::UnloadInCPU(piLog* log, Layer* la)
	{
        // Nothing to do, mSounds will be freed at once on Deinit
	}

	bool LayerRendererSound::LoadInSPU(piSoundEngine* soundEngine, piLog* log, Layer* la)
	{
		if (soundEngine == nullptr) return true;
        if (!la->GetLoaded()) return true;                

        LayerSound* lp = (LayerSound*)la->GetImplementation();
        LayerSound::Type type = lp->GetType();

        const int id = lp->GetGpuId();
        piAssert(id >= 0);
        if (id == -1) return false;

        iSound* snd = (iSound*)mSounds.GetAddress(id);

        if (lp->GetCompressed())
        {
            const piWav *wav = lp->GetSound();
            snd->mSoundID = soundEngine->AddSound(wav->mNumChannels, wav->mDataSize, wav->mData, (type == LayerSound::Type::Positional));
            log->Printf(LT_MESSAGE, L"Add OGG OPUS sound object %s ID=%d chan=%d", la->GetName().GetS(), snd->mSoundID, wav->mNumChannels);
        }
        else
        {
            const piWav *wav = lp->GetSound();
            snd->mSoundID = soundEngine->AddSound(wav->mRate, wav->mBits, wav->mNumChannels, wav->mDataSize, wav->mData, (type == LayerSound::Type::Positional));
            log->Printf(LT_MESSAGE, L"Add WAV sound object %s ID=%d", la->GetName().GetS(), snd->mSoundID);
        }

        if (snd->mSoundID < 0)
		{
			log->Printf(LT_ERROR, L"Sound wave is invalid...(%d)", snd->mSoundID);
            /*
            // Save out an opus file to test
            if (lp->GetCompressed())
            {
                const piWav *wav = lp->GetSound();
                piFile file;
                piString str;
                str.InitConcatW(la->GetName().GetS(), L".opus");
                file.Open(str.GetS(), L"wb");
                file.WriteUInt8array((uint8_t*)wav->mData, wav->mDataSize);
                file.Close();
                str.End();
            }
            */
			return false;            
		}

        // backwards compatibility mode - adjust now
        if (soundEngine->GetType(snd->mSoundID) == piSoundEngine::SoundType::Ambisonic && lp->GetType() != LayerSound::Type::Ambisonic)
        {
            lp->SetType(LayerSound::Type::Ambisonic);
            type = LayerSound::Type::Ambisonic;
        }

        // reset
        snd->mPlayingCached = false;
        snd->mFinalVolumeCached = 0.0f;
        snd->mPausedCached = false;
        snd->mOffsetCached = 0;        
        

        // make sure what we read is what we have!
        piAssert(static_cast<int>(soundEngine->GetType(snd->mSoundID)) == static_cast<int>(lp->GetType()));

        LayerSound::AttenuationParameters atten;
        lp->GetAttenuation(&atten);

        LayerSound::ModifierParameters modif;
        lp->GetModifier(&modif);
        
        if (type == LayerSound::Type::Positional)
        {
            soundEngine->SetAttenMode(snd->mSoundID, static_cast<piSoundEngine::AttenuationType>(atten.mType));
            snd->mAttenuationTypeCached = atten.mType;

            soundEngine->SetAttenMinMax(snd->mSoundID, atten.mMin, atten.mMax);
            snd->mAttenuationMinCached = atten.mMin;
            snd->mAttenuationMaxCached = atten.mMax;

            soundEngine->SetModifierType(snd->mSoundID, static_cast<piSoundEngine::ModifierType>(modif.mType));
            snd->mModifierTypeCached = modif.mType;

            if (modif.mType == LayerSound::ModifierType::DirectionalCone)
            {
                soundEngine->SetModifierCone(snd->mSoundID, modif.mParams.mDirectionalCone.mAngleIn, modif.mParams.mDirectionalCone.mAngleBand, modif.mParams.mDirectionalCone.mAttenOut);
            }
            else if (modif.mType == LayerSound::ModifierType::DirectionalFrus)
            {
                soundEngine->SetModifierFrustum(snd->mSoundID, modif.mParams.mDirectionalFrus.mAngleInX, modif.mParams.mDirectionalFrus.mAngleInY, modif.mParams.mDirectionalFrus.mAngleBand, modif.mParams.mDirectionalFrus.mAttenOut);
            }
        }

        const bool looping = lp->GetLooping();
        soundEngine->SetLooping(snd->mSoundID, looping);
        soundEngine->SetVolume(snd->mSoundID, snd->mFinalVolumeCached);        	


		return true;
	}
	bool LayerRendererSound::UnloadInSPU(piSoundEngine* sound, Layer *la)
	{
		if (sound == nullptr) return true;

		LayerSound* ls = (LayerSound*)la->GetImplementation();
        const int id = ls->GetGpuId();
        if (id == -1) return false;

		iSound* me = (iSound*)mSounds.GetAddress(id);

        if (me->mSoundID == -1) return false;
      
		sound->DelSound(me->mSoundID);
		me->mSoundID = -1;

        return true;
	}

	bool LayerRendererSound::LoadInGPU(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la)
	{
		return true;
	}

	bool LayerRendererSound::UnloadInGPU(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la)
	{
		return true;
	}

   
	void LayerRendererSound::GlobalWork(piRenderer* renderer, piSoundEngine* soundEngine, piLog* log, Layer* la, float masterVolume)
	{
        LayerSound *ls = (LayerSound *) la->GetImplementation();
        if (!la->GetLoaded()) return;
         
        int id = ls->GetGpuId();
        if (id == -1)
        {
            LoadInCPU( log, la);
        }
        id = ls->GetGpuId();
        iSound *me = (iSound *) mSounds.GetAddress(id);

		const bool forceRestart = ls->GetForceRestart();
		const bool playing = ls->GetPlaying();
		const uint64_t offset = ls->GetOffset();
		if (me->mPlayingCached != playing || me->mOffsetCached != offset || forceRestart)
		{
            if (playing)
            {
                if (me->mSoundID < 0)
                    LoadInSPU(soundEngine, log, la);

                // play anyways to enable force restart
                soundEngine->Play(me->mSoundID, offset);                
            }
            else if (me->mSoundID>=0)
            {
                soundEngine->Stop(me->mSoundID);
#ifdef UNLOAD_SOUNDS
                if(!UnloadInSPU(soundEngine, la))                    
                    log->Printf(LT_ERROR, L"Could not unload from SPU layer %s",la->GetName().GetS());
                else
                    log->Printf(LT_ERROR, L"Unloaded from SPU layer %s", la->GetName().GetS());
#endif
            }
			me->mPlayingCached = playing;
			me->mOffsetCached = offset;
			ls->SetForceRestart(false);
		}
		
        if (me->mSoundID < 0) 
            return;

        if (playing)
        {
           if (soundEngine->GetPlaybackState(me->mSoundID) == piSoundEngine::PlaybackState::Stopped)
            {
                // non looping sound has stopped
                ls->SetPlaying(false);
                me->mPlayingCached = false;
#ifdef UNLOAD_SOUNDS
                if (!UnloadInSPU(soundEngine, la))
                    log->Printf(LT_ERROR, L"Could not unload from SPU layer %s", la->GetName().GetS());
                else
                    log->Printf(LT_ERROR, L"Unloaded from SPU layer %s", la->GetName().GetS());
                return;
#endif
            }
        }

        const bool paused = ls->GetPaused();
        if (me->mPausedCached != paused)
        {
            if (paused) soundEngine->Pause(me->mSoundID);
            else        soundEngine->Resume(me->mSoundID);
            me->mPausedCached = paused;
        }
        
        const float modifierVolume = soundEngine->ComputeModifiers(me->mSoundID);
        const float finalVolume = masterVolume * ls->GetVolume() * ls->GetGain() * modifierVolume;

        if (finalVolume != me->mFinalVolumeCached)
        {
            soundEngine->SetVolume(me->mSoundID, finalVolume);
            me->mFinalVolumeCached = finalVolume;
        }

        
        const trans3d layer2world = la->GetTransformToWorld();
        const LayerSound::Type type = ls->GetType();

        if (type == LayerSound::Type::Positional)
        {
            LayerSound::AttenuationParameters atten;
            ls->GetAttenuation(&atten);
            LayerSound::ModifierParameters modif;
            ls->GetModifier(&modif);

            const double scale = getScale(layer2world);

            if (atten.mType != me->mAttenuationTypeCached)
            {
                soundEngine->SetAttenMode(me->mSoundID, static_cast<piSoundEngine::AttenuationType>(atten.mType));
                me->mAttenuationTypeCached = atten.mType;
            }

            const double attenMin = double(atten.mMin) * scale;
            const double attenMax = double(atten.mMax) * scale;
            if (attenMin != me->mAttenuationMinCached || attenMax != me->mAttenuationMaxCached)
            {
                soundEngine->SetAttenMinMax(me->mSoundID, attenMin, attenMax);
                me->mAttenuationMinCached = attenMin;
                me->mAttenuationMaxCached = attenMax;
            }
            //---------

            if (modif.mType != me->mModifierTypeCached)
            {
                soundEngine->SetModifierType(me->mSoundID, static_cast<piSoundEngine::ModifierType>(modif.mType));
                me->mModifierTypeCached = modif.mType;
                if (modif.mType == LayerSound::ModifierType::DirectionalCone)
                {
                    soundEngine->SetModifierCone(me->mSoundID, modif.mParams.mDirectionalCone.mAngleIn, modif.mParams.mDirectionalCone.mAngleBand, modif.mParams.mDirectionalCone.mAttenOut);
                }
                else if (modif.mType == LayerSound::ModifierType::DirectionalFrus)
                {
                    soundEngine->SetModifierFrustum(me->mSoundID, modif.mParams.mDirectionalFrus.mAngleInX, modif.mParams.mDirectionalFrus.mAngleInY, modif.mParams.mDirectionalFrus.mAngleBand, modif.mParams.mDirectionalFrus.mAttenOut);
                }

            }

            if (modif.mType == LayerSound::ModifierType::DirectionalCone)
            {
                const double angleIn = double(modif.mParams.mDirectionalCone.mAngleIn);
                const double angleBand = double(modif.mParams.mDirectionalCone.mAngleBand);
                const double attenOut = double(modif.mParams.mDirectionalCone.mAttenOut);
                if (angleIn != me->mModifierConeAngleInner || angleBand != me->mModifierConeAngleBand || attenOut != me->mModifierConeAttenOut)
                {
                    soundEngine->SetModifierCone(me->mSoundID, angleIn, angleBand, attenOut);
                    me->mModifierConeAngleInner = angleIn;
                    me->mModifierConeAngleBand = angleBand;
                    me->mModifierConeAttenOut = attenOut;
                }
            }
            else if (modif.mType == LayerSound::ModifierType::DirectionalFrus)
            {
                const double angleInX = double(modif.mParams.mDirectionalFrus.mAngleInX);
                const double angleInY = double(modif.mParams.mDirectionalFrus.mAngleInY);
                const double angleBand = double(modif.mParams.mDirectionalFrus.mAngleBand);
                const double attenOut = double(modif.mParams.mDirectionalFrus.mAttenOut);
                if (angleInX != me->mModifierFrustumAngleInnerX || angleInY != me->mModifierFrustumAngleInnerY || angleBand != me->mModifierFrustumAngleBand || attenOut != me->mModifierFrustumAttenOut)
                {
                    soundEngine->SetModifierFrustum(me->mSoundID, angleInX, angleInY, angleBand, attenOut);
                    me->mModifierFrustumAngleInnerX = angleInX;
                    me->mModifierFrustumAngleInnerY = angleInY;
                    me->mModifierFrustumAngleBand = angleBand;
                    me->mModifierFrustumAttenOut = attenOut;
                }
            }

            const vec3d wPos = layer2world.mTranslation;//(layer2world * vec4d(0.0, 0.0, 0.0, 1.0)).xyz();
            const vec3d wDir = normalize((layer2world * vec4d(0, 0, -1, 0)).xyz());
            const vec3d wUp = normalize((layer2world * vec4d(0, 1, 0, 0)).xyz());
            soundEngine->SetPositionOrientation(me->mSoundID, (double*)&wPos, (double*)&wDir, (double*)&wUp);
        }
        else if (type == LayerSound::Type::Ambisonic)
        {
            const vec3d wDir = normalize((layer2world * vec4d(0, 0, -1, 0)).xyz());
            const vec3d wUp = normalize((layer2world * vec4d(0, 1, 0, 0)).xyz());
            soundEngine->SetOrientation(me->mSoundID, (double*)&wDir, (double*)&wUp);
        }
        
	}

	void LayerRendererSound::PrepareForDisplay(StereoMode stereoMode)
	{
	}

	void LayerRendererSound::DisplayPreRender(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la, const frustum3& frus, const trans3d & layerToViewer, float opacity)
	{
	}

	void LayerRendererSound::DisplayRender(piRenderer* renderer, piLog* log, piBuffer layerStateShaderConstans, int capDelta)
	{
	}
    
    
}
