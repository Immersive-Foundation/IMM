#pragma once

#include "../libBasics/piTypes.h"
#include "../libBasics/piVecTypes.h"

namespace ImmCore {

class piSoundEngine
{
public:
    enum class PlaybackState
    {
        PlayingStarting = 0,
        Playing = 1,

        PausingStarted = 2,
        Paused = 3,

        StoppingStarted = 3,
        Stopped = 4
    };

    enum class AttenuationType
    {
        None = 0,
        Linear = 1,
        Logarithmic = 2,
        InverseSquare = 3,
        Num = 4
    };

    enum class ModifierType
    {
        None = 0,
        Cone = 1,
        Frus = 2,
        Num = 3
    };

    enum class SoundType
    {
        Flat = 0,       // Headlock/traditional
        Ambisonic = 1,  // 360
        Positional = 2, // 3D
        Num = 3
    };
    

	// Need virtual destructor for proper deletion of piSoundEngineAudioSDKBackend which derives from piSoundEngine.
	virtual ~piSoundEngine() {}

	virtual void  PauseAllSounds(void) = 0;
	virtual void  ResumeAllSounds(void) = 0;
    virtual void  SetListener(const trans3d & listenerToWorld) = 0;

    virtual int   AddSound(void(*callback)(float* channelBuffer, size_t numSamples, size_t numChannels, void* userData) , void * userData) = 0;
	virtual int   AddSound(const wchar_t *filename, bool makePositional) = 0; // wav/mp3 file
	virtual int   AddSound(const int frequency, int precision, int numChannels, uint64_t length, const void *buffer, bool makePositional) = 0;
    virtual int   AddSound(int numChannels, uint64_t length, const void *buffer, bool makePositional) = 0;
	virtual void  DelSound(int id) = 0;


	virtual bool  Play(int id, uint64_t microSecondOffset) = 0;
	virtual bool  Stop(int id) = 0;
	virtual bool  Pause(int id) = 0;
	virtual bool  Resume(int id) = 0;
    virtual PlaybackState GetPlaybackState(int id) = 0;

	//virtual bool  GetIsPlaying(int id) = 0;
	virtual float GetVolume(int id) const = 0;
	virtual bool  SetVolume(int id, float volume) = 0;
	virtual bool  GetLooping(int id) const = 0;
	virtual void  SetLooping(int id, bool looping) = 0;

    // promote from Flat to Positional,e tc
    virtual SoundType GetType(int id) const = 0;
    virtual bool  ConvertType(int id, SoundType type) = 0;
    virtual bool  CanConvertType(int id, SoundType type) const = 0;

    // only for Positional sounds
    virtual void  SetAttenMode(int id, AttenuationType attenMode) = 0;
	virtual void  SetAttenMinMax(int id, double attenMin, double attenMax) = 0;
    virtual void  SetPosition(int id, const double * pos) = 0;
    virtual void  SetOrientation(int id, const double * dir, const double * up) = 0;
    virtual void  SetPositionOrientation(int id, const double * pos, const double * dir, const double * up) = 0;
    virtual void  SetModifierType(int id, ModifierType modifType) = 0;
    virtual void  SetModifierCone(int id, double angleInner, double angleBand, double attenOut ) = 0;
    virtual void  SetModifierFrustum(int id, double angleInnerX, double angleInnerY, double angleBand, double attenOut) = 0;
    virtual float ComputeModifiers(int id) = 0;

};

} // namespace ImmCore
