#pragma once

#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libBasics/piImage.h"
#include "libImmCore/src/libBasics/piVecTypes.h"
#include "layer.h"

#define DEFAULT_LAYER_SPAWNAREA_VERSION 1

namespace ImmExporter
{
    class LayerSpawnArea
    {
    public:
        enum class TrackingLevel : uint32_t
        {
            Floor = 0,
            Eye = 1
        };

        struct Volume
        {
            enum class Type : uint32_t
            {
                Sphere = 0,
                Box = 1
            };
            struct Shape
            {
                ImmCore::vec4   mSphere;
                ImmCore::bound3 mBox;
            };

            Type mType;
            Shape mShape;

            bool  mAllowTranslationX;
            bool  mAllowTranslationY;
            bool  mAllowTranslationZ;
        };


        LayerSpawnArea();
        ~LayerSpawnArea();

        bool Init(uint32_t version = DEFAULT_LAYER_SPAWNAREA_VERSION);
        void Deinit(void);

        const int GetGpuId(void) const;
        void SetGpuId(int id);

        const int      GetVersion(void) const;
        void  SetTracking(TrackingLevel tracking);
        const TrackingLevel GetTracking(void) const;
        void SetVolume(Volume vol);
        const Volume & GetVolume(void) const;
        const ImmCore::piImage* GetScreenshot(void) const;
        const Volume::Type& GetVolumeType(void) const;
        const ImmCore::vec4& GetVolumeSphere(void) const;
        const ImmCore::bound3& GetVolumeBox(void) const;

        bool AssignAsset(const ImmCore::piImage *asset, bool move);
        inline bool GetTranslationX(void) const { return mVolume.mAllowTranslationX; }
        inline bool GetTranslationY(void) const { return mVolume.mAllowTranslationY; }
        inline bool GetTranslationZ(void) const { return mVolume.mAllowTranslationZ; }

    private:

        int         mVersion;
        TrackingLevel mTracking;
        Volume      mVolume;
        ImmCore::piImage mScreenshot;
        bool mHasAsset;
    };
}
