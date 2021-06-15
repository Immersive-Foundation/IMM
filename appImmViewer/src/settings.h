#pragma once

#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libBasics/piStr.h"
#include "libCore/src/libBasics/piTypes.h"
#include "libCore/src/libBasics/piTArray.h"

namespace ExePlayer
{
    class Settings
    {
    public:
        struct Files
        {
            ImmCore::piTArray<ImmCore::piString> mLoad;
        } mFiles;

        struct Window
        {
            bool mFullScreen;
            int mPositionX;
            int mPositionY;
            int mWidth;
            int mHeight;
        } mWindow;

        struct Playback
        {
            ImmCore::trans3d mLocation;			// in world coordinates. This is normally identity in Spaces and our normal player
            struct
            {
                ImmCore::piString mLocation;
                ImmCore::trans3d mCustom;
            }mPlayerSpawn;
        }mPlayback;

        struct Sound
        {
            ImmCore::piString mDevice;
        } mSound;

        struct  UI
        {
            bool mEnableHaptics;
            bool mLeftHanded;
            float UISoundVolume;
        }mUI;


        struct Rendering
        {
            enum class API
            {
                GL = 0,
                DX = 1,
                GLES = 2
            };

            enum Technique : int
            {
                Static = 0,
                Pretessellated = 1
            };

            API        mRenderingAPI;
            Technique  mRenderingTechnique;
            bool mEnableVR;
            float mPixelDensity;
            int mSupersampling;
        } mRendering;

    public:
        Settings();
        ~Settings();

        bool Init(const wchar_t* file, ImmCore::piLog* log);
        void End(void);
    };
}
