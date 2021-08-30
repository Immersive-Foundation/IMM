#pragma once

#include "libImmCore/src/libBasics/piString.h"
#include "libImmCore/src/libBasics/piVecTypes.h"
#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libWave/piWave.h"

namespace ImmImporter
{

	class LayerSound
	{
	public:
        enum class Type : uint32_t
        {
            Flat = 0,       // Headlock/traditional
            Ambisonic = 1,  // 360
            Positional = 2, // 3D        
            Num = 3
        };

        enum class AttenuationType : uint32_t
        {
            None = 0,
            Linear = 1,
            Logarithmic = 2,
            //InverseSquare = 3
            Num = 3
        };

        struct AttenuationParameters
        {
            AttenuationType mType;
            float           mMin;
            float           mMax;
        };

        // Note. I think the Attenuation above should be just one more type of modifier, not it's own thing
        enum class ModifierType : uint32_t
        {
            None = 0,
            DirectionalCone = 1,      // Directional decay in a circular  shape, only for "Positional" sounds
            DirectionalFrus = 2,      // Directional decay in a rectangle shape, only for "Positional" sounds
            Num = 3
        };

        struct ModifierParameters
        {
            ModifierType mType;
            struct // if i make this a union, the constructors have issues (is there a way I can tell the compiler which union member needs to win?)
            {
                struct
                {
                    float mAngleIn;   // in radians, convert to degrees for user input/UI purposes
                    float mAngleBand; // in radians. angleOut will be angleIn = angleBand
                    float mAttenOut;  // value of the attenuation at angleOut
                }mDirectionalCone;
                struct
                {
                    float mAngleInX;
                    float mAngleInY;
                    float mAngleBand; // in radians
                    float mAttenOut;  // value of the attenuation at angleOut
                }mDirectionalFrus;
            }mParams;
        };


		LayerSound();
		~LayerSound();

		bool Init(bool loop, bool play, float gain, 
                  const AttenuationParameters & attenParams,
                  const ModifierParameters & modifParams,
                  Type type, float duration, ImmCore::piLog *log);
		void Deinit(void);

		void SetPlaying(bool value, uint64_t microseconds = 0);
        void SetPaused(bool value);
        void SetVolume(float value);
        void SetType(Type type); // only so we are able to fix old drawings and backwards compatibiliyu. Otherwise, architecturally it doesn't have sense for this function ot exist
        void SetForceRestart(bool force);
            
        float GetGain(void) const { return mGain; }
        float GetVolume(void)const;
        Type GetType(void) const;
        void GetAttenuation(AttenuationParameters *res) const;
        void GetModifier(ModifierParameters *res) const;

        uint64_t GetOffset(void) const { return mOffset; }
        inline bool GetLooping(void) const { return mLoop; }
		inline bool GetPlaying(void) const { return mPlay; }
        inline bool GetPaused(void) const { return mPaused; }
        inline bool GetForceRestart(void) const { return mForceRestart; }
        inline bool GetCompressed(void) { return mCompressed; }
//		inline float GetDuration(void) const { return mDuration; }

        inline void SetCompressed(bool c) { mCompressed = c; }
        inline ImmCore::piWav * GetSound(void) { return &mSound; }

		int GetGpuId(void) const;
		void SetGpuId(int id);

	private:
        Type     mType;
        ImmCore::piWav    mSound;
        

        AttenuationParameters mAttenuation;
        ModifierParameters mModifier;
        float    mGain;
		float    mDuration;
		bool     mLoop;
        bool     mForceRestart;
        // these are ot to be serialized, these are runtime playback/animation controls
        uint64_t mOffset;
        bool     mPaused;
        bool     mPlay;
        bool     mCompressed;
        float    mVolume;

		int mGpuId;
	};

}
