#pragma once

#include "libCore/src/libBasics/piVecTypes.h"

namespace ImmPlayer
{

    enum class StereoMode : int
    {
        None = 0,        // mono
        Fallback = 1,    // fallback in old gfx cards, manual stereo
        Preferred = 2    // default mode, based on instancing currently
    };

    enum class ClipSpaceDepth : int
    {
        FromNegativeOneToOne = 0,
        FromZeroToOne = 1
    };

    enum class DepthBuffer : int
    {
        Linear01 = 0,  // traditional zbuffer, set depth compares to less_than
        Linear10 = 1,  // reverse zbuffer, set depth compares to greater_than
    };


    inline ImmCore::mat4x4 transForGPU(const ImmCore::trans3d & t) { return d2f(toMatrix(t)); }

    typedef struct
    {
        ImmCore::mat4x4 mLayerToViewer;
        float  mLayerToViewerScale;
        float  mOpacity;
        float  mFlipSign;
        float  mDrawInTime;
        ImmCore::vec4   mAnimParam; // draw-in and other effect parameters
        union
        {
            struct
            {
                float dummy0;
                float dummy1;
                float dummy2;
                float dummy3;
                float dummy4;
                float dummy5;
                float dummy6;
                float dummy7;
            }mNone;
            struct
            {
                float mFrequency;
                float mSpeed;
                float mAmplitude;
                float dummy3;
                float dummy4;
                float dummy5;
                float dummy6;
                float dummy7;
            }mWiggle;
            struct
            {
                int   mWaveForm;
                float mSpeed;
                float mMinOut;
                float mMaxOut;
                float mMinIn;
                float mMaxIn;
                float dummy6;
                float dummy7;
            }mBlink;
        }mKeepAlive; // always 8 floats
        uint32_t mID;

        void setLayerToViewerInfo(const ImmCore::trans3d & layerToViewer)
        {
            const double layerToViewerScale = getScale(layerToViewer);
            // okey to cast to floats in view space, for GPU
            mLayerToViewer = transForGPU(layerToViewer);
            mLayerToViewerScale = static_cast<float>(layerToViewerScale);
        }

    } LayersState; // slot 3

}
