#pragma once

#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libWave/piWave.h"
#include "layer.h"

#define DEFAULT_LAYER_SOUND_VERSION 1

namespace ImmExporter
{

	class LayerSound
	{
	public:
        enum class SpatialType : uint32_t
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

		bool Init(uint32_t version = DEFAULT_LAYER_SOUND_VERSION);
		void Deinit();

        SpatialType GetType() const;
        void SetSpatialType(SpatialType type); // only so we are able to fix old drawings and backwards compatibiliyu. Otherwise, architecturally it doesn't have sense for this function ot exist

        void SetGain(float gain);
        inline const float GetGain(void) const { return mGain;}

        void SetVolume(float value);
        const float GetVolume()const;

        void SetAttenuation(AttenuationParameters attenParams);
        const void GetAttenuation(AttenuationParameters *attenParams) const;

        void SetModifier(ModifierParameters modifParams);
        const void GetModifier(ModifierParameters *res) const;

        void SetLooping(bool loop);
        inline const bool GetLooping() const { return mLoop; }

        inline ImmCore::piWav * GetSound() { return &mSound; }

        void AssignAsset(const ImmCore::piWav* wav, bool move);
	private:
        // Serialization Data
        SpatialType     mType;
        AttenuationParameters mAttenuation;
        ModifierParameters mModifier;
        float    mGain;
		float    mDuration;
		bool     mLoop;
        float    mVolume; //TODO add to file

        // Serialization Asset
        ImmCore::piWav    mSound;
        uint32_t mVersion;
        bool mHasAsset;

	};

}
