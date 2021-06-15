#pragma once

#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libRender/piRenderer.h"
#include "libCore/src/libBasics/piTypes.h"

namespace ExePlayer
{
    class Resolve
    {
    private:
        ImmCore::piRasterState mRenderStateResolve;
        ImmCore::piShader mAAResolveShader;
        ImmCore::piBlendState mBlendStateNone;

    public:

        bool Init(ImmCore::piRenderer* renderer, int superSample);
        void DeInit(ImmCore::piRenderer* renderer);
        void Do(ImmCore::piRenderer* renderer, ImmCore::piRTarget target, const int *vp, const int unXOffset, const float fade, ImmCore::piTexture colorTextureM);
    };
}
