#pragma once

#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libBasics/piImage.h"
#include "layer.h"

#define DEFAULT_LAYER_SPAWNAREA_VERSION 1

namespace ImmImporter
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

        bool Init(const Volume & volume, const TrackingLevel & tracking, uint32_t version = DEFAULT_LAYER_SPAWNAREA_VERSION);
        void Deinit(void);

        const int GetGpuId(void) const;
        void SetGpuId(int id);

        const int      GetVersion(void) const;
        const TrackingLevel GetTracking(void) const;
        const Volume & GetVolume(void) const;
        const ImmCore::piImage* GetScreenshot(void) const;
        const Volume::Type& GetVolumeType(void) const;
        const ImmCore::vec4& GetVolumeSphere(void) const;
        const ImmCore::bound3& GetVolumeBox(void) const;

        bool LoadAssetMemory(const ImmCore::piTArray<uint8_t>& data, ImmCore::piLog* log, const wchar_t* ext);
        inline bool GetTranslationX(void) const { return mVolume.mAllowTranslationX; }
        inline bool GetTranslationY(void) const { return mVolume.mAllowTranslationY; }
        inline bool GetTranslationZ(void) const { return mVolume.mAllowTranslationZ; }
        const AssetFormat GetAssetFormat() const;

    private:

        int         mVersion;
        int         mGpuId;
        TrackingLevel mTracking;
        Volume      mVolume;
        ImmCore::piImage     mScreenshot;
        bool        mHasAsset;
        AssetFormat mFormat;
    };
}
