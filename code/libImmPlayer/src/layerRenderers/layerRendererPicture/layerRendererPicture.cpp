//#define RENDER_BUDGET

#if defined(WINDOWS)
// enable this for to workaround a bug in the NVidia cards, where glGenerateMimaps() corrupts
// the viewport state
#define NVIDIA_BUG_WORKAROUND
#endif

#ifdef RENDER_BUDGET
#include <algorithm>
#endif

#include "libImmCore/src/libBasics/piArray.h"
#include "libImmCore/src/libBasics/piPool.h"
#include "libImmCore/src/libBasics/piString.h"
#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libRender/piRenderer.h"
#include "libImmCore/src/libSound/piSound.h"
#include "libImmCore/src/libBasics/piStr.h"

#include "libImmCore/src/libMesh/piMesh.h"
#include "libImmCore/src/libMesh/piRenderMesh.h"
#include "../../blue_noise.h"

#include "layerRendererPicture.h"

using namespace ImmCore;
using namespace ImmImporter;

namespace ImmPlayer
{

    typedef struct
    {
        bool		mFrontFaces;
        LayersState mLayerState;
        LayerPicture::ContentType mType;
        piTexture	mTexture;
        piSampler   mSampler;
        float       mAspectRatio;
        ivec3       mRes;
        piImage * mImage;
        bool        mUploaded;
        #ifdef RENDER_BUDGET
        float        mDistance;
        #endif
    }iLayerDrawInfo;

    LayerRendererPicture::LayerRendererPicture() : LayerRenderer() {}
    LayerRendererPicture::~LayerRendererPicture() {}

#if defined(WINDOWS)
#include "tmp/shader_pip360Equirect_vs_hlsl.inc"
#include "tmp/shader_pip360Equirect_fs_hlsl.inc"
#include "tmp/shader_pi2D_vs_hlsl.inc"
#include "tmp/shader_pi2D_fs_hlsl.inc"
#include "shader_pi2D_vs.glsl"
#include "shader_pi2D_fs.glsl"
#include "shader_pip360Cubemap_vs.glsl"
#include "shader_pip360Cubemap_fs.glsl"
#include "shader_pip360Equirect_vs.glsl"
#include "shader_pip360Equirect_fs.glsl"
#elif defined(ANDROID)
#include "shader_pi2D_vs.es.glsl"
#include "shader_pi2D_fs.es.glsl"
#include "shader_pip360Cubemap_vs.es.glsl"
#include "shader_pip360Cubemap_fs.es.glsl"
#include "shader_pip360Equirect_vs.es.glsl"
#include "shader_pip360Equirect_fs.es.glsl"
#endif

    // Creates a cube with 6 sides of 16x16 quads each. Note, the sides do NOT share the verties
    // along the edges of the cube. They could, but for the purposes of generating our sky dome
    // it's not very important. If we ever see cracks along the edges, we can revisit this construction
    // code and connect faces appropriatelly
    bool iGenerateDome(piMesh *me, const piMesh::VertexFormat *vertexFormat)
    {
        const int res = 16;

        if (!me->Init(1, 6*(res+1)*(res+1), vertexFormat, piMesh::Type::Polys, 1, 6*res*res))
            return false;

        for( int s=0; s<6; s++ )
        {
            const int vbase = s*(res+1)*(res+1);
            const int fbase = s*res*res;

            for( int u=0; u<(res+1); u++ )
            for( int v=0; v<(res+1); v++ )
            {
                const int vid = vbase + (res+1)*v + u;
                const float x = -1.0f + 2.0f*float(u)/float(res);
                const float y = -1.0f + 2.0f*float(v)/float(res);
                vec3 vertex = vec3(0.0f);
                     if (s == 0) { vertex = vec3(-1.0f,  y,   x); }
                else if (s == 1) { vertex = vec3( 1.0f,  x,   y); }
                else if (s == 2) { vertex = vec3(   x,-1.0f,  y); }
                else if (s == 3) { vertex = vec3(   y, 1.0f,  x); }
                else if (s == 4) { vertex = vec3(   y,   x,-1.0f); }
                else             { vertex = vec3(   x,   y, 1.0f); }
                vertex = normalize( vertex );
                *((vec3*)me->GetVertexData(0, vid, 0)) = vertex;
                *((vec3*)me->GetVertexData(0, vid, 1)) = vertex;
            }
            piMesh::Face32 *face = me->mFaceData.mIndexArray[0].mBuffer + fbase;
            for( int u=0; u<res; u++ )
            for( int v=0; v<res; v++ )
            {
                face->mNum = 4;
                face->mIndex[0] = vbase + (res+1)*(v+0) + (u+0);
                face->mIndex[1] = vbase + (res+1)*(v+0) + (u+1);
                face->mIndex[2] = vbase + (res+1)*(v+1) + (u+1);
                face->mIndex[3] = vbase + (res+1)*(v+1) + (u+0);
                face++;
            }
        }

        return true;
    }

    bool piGenerateCube(piMesh *me, const piMesh::VertexFormat *vertexFormat)
    {
        if (!me->Init(1, 8, vertexFormat, piMesh::Type::Polys, 1, 6))
            return false;

        for (int i = 0; i < 8; i++)
        {
            float *v = (float*)me->GetVertexData(0, i, 0);
            v[0] = -1.0f + 2.0f*((i >> 0) & 1);
            v[1] = -1.0f + 2.0f*((i >> 1) & 1);
            v[2] = -1.0f + 2.0f*((i >> 2) & 1);
        }


        static const char cvd[] = {
            0, 1, 5, 4,
            2, 6, 7, 3,
            1, 3, 7, 5,
            0, 4, 6, 2,
            4, 5, 7, 6,
            0, 2, 3, 1 };

        piMesh::Face32 *face = me->mFaceData.mIndexArray[0].mBuffer;
        for (int i = 0; i < 6; i++)
        {
            face->mNum = 4;
            face->mIndex[0] = cvd[4 * i + 0];
            face->mIndex[1] = cvd[4 * i + 1];
            face->mIndex[2] = cvd[4 * i + 2];
            face->mIndex[3] = cvd[4 * i + 3];
            face++;
        }

        return true;
    }


    bool LayerRendererPicture::Init(piRenderer* renderer, piLog* log, Drawing::ColorSpace colorSpace, bool frontIsCCW)
    {
        if (!mLayerInfo.Init(128, sizeof(iLayerDrawInfo))) // one per layer
            return false;

        if (!mVisibleLayerInfos.Init(128, sizeof(uint32_t)))
            return false;


        // SHADERS ---------------------------------------------------------
        for (int j = 0; j < 4; ++j) // debug render modes: normal, wireframe, overdraw, wireframe + overdraw
        for (int i = 0; i < 3; i++)
        {
            if (renderer->GetAPI() == piRenderer::API::GL && static_cast<StereoMode>(i) == StereoMode::Preferred &&
                (!renderer->SupportsFeature(piRenderer::RendererFeature::VIEWPORT_ARRAY) ||
                    !renderer->SupportsFeature(piRenderer::RendererFeature::VERTEX_VIEWPORT)
                    )
                )
            {
                // skip compiling fast stereo shaders when we don't support the feature
                continue;
            }

            char error[2048];
#if !defined(ANDROID)
            if (j >= 1) continue; // skip wireframe variants for PC
#else
            if (/*mDebugRenderMode == Disabled &&*/ j > 0) continue; // skip debug shaders if disabled
#endif

            int idx = j * 3 + i;

            if (renderer->GetAPI() == piRenderer::API::GL || renderer->GetAPI() == piRenderer::API::GLES)
            {
                // 2D
                {
                    const piShaderOptions opts = { 3,{
                        {"COLOR_SPACE", static_cast<int>(colorSpace) },
                        { "STEREOMODE", i },
                        { "DEBUG_RENDER_MODE", j },
                        } };

                    mShaders[idx][LayerPicture::Image2D] = renderer->CreateShader(&opts, shader_pi2D_vs, nullptr, nullptr, nullptr, shader_pi2D_fs, error);
                    if (!mShaders[idx][LayerPicture::Image2D])
                    {
                        log->Printf(LT_ERROR, L"Could not initalize 2d image layer shader\n%s", pistr2ws(error));
                        return false;
                    }
                }

                // 360 Equirect mono
                {
                    const piShaderOptions opts = { 4,{
                        { "COLOR_SPACE", static_cast<int>(colorSpace) },
                        { "STEREOMODE",i },
                        { "FORMAT_IS_STEREO", 0 },
                        { "DEBUG_RENDER_MODE", j },
                    } };

                    mShaders[idx][LayerPicture::Image360EquirectMono] = renderer->CreateShader(&opts, shader_pip360Equirect_vs, nullptr, nullptr, nullptr, shader_pip360Equirect_fs, error);
                    if (!mShaders[idx][LayerPicture::Image360EquirectMono])
                    {
                        log->Printf(LT_ERROR, L"Could not initalize 360 equirect mono image layer shader\n%s", pistr2ws(error));
                        return false;
                    }

                }

                // 360 Equirect stereo
                {
                    const piShaderOptions opts = { 4,{
                        { "COLOR_SPACE", static_cast<int>(colorSpace) },
                        { "STEREOMODE", i },
                        { "FORMAT_IS_STEREO", 1 },
                        { "DEBUG_RENDER_MODE", j },
                    } };

                    mShaders[idx][LayerPicture::Image360EquirectStereo] = renderer->CreateShader(&opts, shader_pip360Equirect_vs, nullptr, nullptr, nullptr, shader_pip360Equirect_fs, error);
                    if (!mShaders[idx][LayerPicture::Image360EquirectStereo])
                    {
                        log->Printf(LT_ERROR, L"Could not initalize 360 equirect stereo image layer shader\n%s", pistr2ws(error));
                        return false;
                    }
                }

                // 360 Cubemap mono
                {
                    const piShaderOptions opts = { 3,{
                        { "COLOR_SPACE", static_cast<int>(colorSpace) },
                        { "STEREOMODE", i },
                        { "DEBUG_RENDER_MODE", j },
                    } };

                    mShaders[idx][LayerPicture::Image360CubemapCrossMono] = renderer->CreateShader(&opts, shader_pip360Cubemap_vs, nullptr, nullptr, nullptr, shader_pip360Cubemap_fs, error);
                    if (!mShaders[idx][LayerPicture::Image360CubemapCrossMono])
                    {
                        log->Printf(LT_ERROR, L"Could not initalize 360 cubemap mono image layer shader\n%s", pistr2ws(error));
                        return false;
                    }
                    // reuse shader for Vstrip
                    mShaders[i][LayerPicture::Image360CubemapVstripMono] = mShaders[i][LayerPicture::Image360CubemapCrossMono];
                }
            }
            else
            {
#ifndef ANDROID
                const int poff = 3 * (static_cast<int>(colorSpace));

                mShaders[idx][LayerPicture::Image2D] = renderer->CreateShaderBinary(nullptr, shader_pi2D_vs_code[i], shader_pi2D_vs_size[i], nullptr, 0, nullptr, 0, nullptr, 0, shader_pi2D_fs_code[i + poff], shader_pi2D_fs_size[i + poff], error);
                if (!mShaders[i][LayerPicture::Image2D])
                {
                    log->Printf(LT_ERROR, L"Could not initalize 2D mono image layer shader\n%s", pistr2ws(error));
                    return false;
                }


                mShaders[idx][LayerPicture::Image360EquirectMono] = renderer->CreateShaderBinary(nullptr, shader_pip360Equirect_vs_code[i], shader_pip360Equirect_vs_size[i], nullptr, 0, nullptr, 0, nullptr, 0, shader_pip360Equirect_fs_code[i + poff], shader_pip360Equirect_fs_size[i + poff], error);
                if (!mShaders[i][LayerPicture::Image360EquirectMono])
                {
                    log->Printf(LT_ERROR, L"Could not initalize 360 equirect mono image layer shader\n%s", pistr2ws(error));
                    return false;
                }
#endif
            }

        } // stereo mode loop


        // GEOMETRY -------------------------------------------------------------------------------------------------------------------

        // Create 360 equirect sphere
        {
            const piMesh::VertexFormat lay1 = { 6 * sizeof(float), 2, 0,{ { 3, piMesh::VertexElemDataType::Float, false },{ 3, piMesh::VertexElemDataType::Float, false } } };

            piMesh mesh;
            if( !iGenerateDome( &mesh, &lay1))
                return false;

            if (renderer->GetAPI() == piRenderer::API::GL || renderer->GetAPI() == piRenderer::API::GLES)
            {
                if (!m360SphereRenderMesh.InitFromMesh(renderer, &mesh, piRenderer::PrimitiveType::Triangle, nullptr))
                {
                    log->Printf(LT_ERROR, L"Could not initalize 360 sphere render mesh in GL");
                    return false;
                }
            }
            else
            {
#ifndef ANDROID
                const void *shaders[3] = { (void*)shader_pip360Equirect_vs0, (void*)shader_pip360Equirect_vs1, (void*)shader_pip360Equirect_vs2 };
                if (!m360SphereRenderMesh.InitFromMeshWithShader(renderer, &mesh, piRenderer::PrimitiveType::Triangle, 3, shaders, shader_pip360Equirect_vs_size, log))
                {
                    log->Printf(LT_ERROR, L"Could not initalize 360 sphere render mesh");
                    return false;
                }
#endif
            }

            mesh.DeInit();
        }

        // Create 360 cubemap cube
        {
            const piMesh::VertexFormat lay1 = { 6 * sizeof(float), 2, 0,{ { 3, piMesh::VertexElemDataType::Float, false },{ 3, piMesh::VertexElemDataType::Float, false } } };

            piMesh mesh;

            if (!piGenerateCube(&mesh, &lay1))
                return false;

            mesh.Normalize(0, 0, 1);

            if (renderer->GetAPI() == piRenderer::API::GL || renderer->GetAPI() == piRenderer::API::GLES)
            {
                if (!m360CubemapRenderMesh.InitFromMesh(renderer, &mesh, piRenderer::PrimitiveType::Triangle, nullptr))
                {
                    log->Printf(LT_ERROR, L"Could not initalize 360 cubemap render mesh");
                    return false;
                }
            }
            else
            {
#ifndef ANDROID
                const void *shaders[3] = { (void*)shader_pip360Equirect_vs0, (void*)shader_pip360Equirect_vs1, (void*)shader_pip360Equirect_vs2 };
                if (!m360CubemapRenderMesh.InitFromMeshWithShader(renderer, &mesh, piRenderer::PrimitiveType::Triangle, 3, shaders, shader_pip360Equirect_vs_size, log))
                {
                    log->Printf(LT_ERROR, L"Could not initalize 360 sphere render mesh");
                    return false;
                }
#endif
            }
            mesh.DeInit();
        }

        mRasterState = renderer->CreateRasterState(false, true, piRenderer::CullMode::NONE, true, false);
        if (!mRasterState) return false;

        mShaderConstants = renderer->CreateBuffer(nullptr, 4*sizeof(float), piRenderer::BufferType::Dynamic, piRenderer::BufferUse::Constant);
        if( !mShaderConstants)
            return false;

        const piRenderer::TextureInfo infob = { piRenderer::TextureType::T2D_ARRAY, piRenderer::Format::C1_8_UNORM, 64, 64, 64, 1 };
        mBlueNoise = renderer->CreateTexture(0, &infob, false, piRenderer::TextureFilter::NONE, piRenderer::TextureWrap::REPEAT, 1.0f, (void*)GetBlueNoise_64x64x64());

        return true;
    }

    void LayerRendererPicture::Deinit(piRenderer* renderer, piLog* log)
    {
        // Verify everything is freed for memory leaks
        bool notDeleted = false;
        bool stillUploaded = false;
        for (uint64_t j = 0; j < mLayerInfo.GetMaxLength(); j++)
        {
            iLayerDrawInfo* me = (iLayerDrawInfo*)mLayerInfo.GetAddress(j);
            if (mLayerInfo.IsUsed(j))
            {
                notDeleted = true;
                if (me->mUploaded)            stillUploaded = true;
            }
        }
        piAssert(notDeleted == false);
        piAssert(stillUploaded == false);

        mVisibleLayerInfos.End();

        m360SphereRenderMesh.End(renderer);
        m360CubemapRenderMesh.End(renderer);

        renderer->DestroyRasterState(mRasterState);

        renderer->DestroyTexture(mBlueNoise);

        renderer->DestroyBuffer(mShaderConstants);

        if (renderer->GetAPI() == piRenderer::API::DX)
        {
            mLayerInfo.End();
            return;
        }
#if defined(ANDROID)
        int num = /*mDebugRenderMode == Disabled ? */3 /*:3 * 4*/;
#else
        int num = 3;
#endif
        for (int i = 0; i < num; i++)
        {
            renderer->DestroyShader(mShaders[i][LayerPicture::Image2D]);
            renderer->DestroyShader(mShaders[i][LayerPicture::Image360EquirectMono]);
            renderer->DestroyShader(mShaders[i][LayerPicture::Image360EquirectStereo]);
            renderer->DestroyShader(mShaders[i][LayerPicture::Image360CubemapCrossMono]);
        }

        mLayerInfo.End();
    }



    bool LayerRendererPicture::LoadInCPU(piLog* log, Layer* la)
    {
        if (!la->GetLoaded()) return true;

        LayerPicture* lp = (LayerPicture*)la->GetImplementation();

        bool isNew = false;
        uint64_t id = -1;
        iLayerDrawInfo* pic = (iLayerDrawInfo*)mLayerInfo.Alloc(&isNew, &id, true);
        if (!pic)
            return false;

        piImage * image = lp->GetImage();

        pic->mAspectRatio = float(image->GetXRes()) / float(image->GetYRes());
        pic->mRes = ivec3(image->GetXRes(), image->GetYRes(), 1);
        pic->mUploaded = false;
        pic->mImage = image;
        pic->mTexture = nullptr;
        pic->mType = lp->GetType();

        lp->SetGpuId(static_cast<int>(id));

        return true;
    }
    void LayerRendererPicture::UnloadInCPU(piLog* log, Layer* la)
    {
        const LayerPicture* lp = (LayerPicture*)la->GetImplementation();
        const uint64_t id = lp->GetGpuId();
        if (id == -1) return;
        mLayerInfo.Free(id);
    }


    bool LayerRendererPicture::UnloadInGPU(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la)
    {
        const LayerPicture* lp = (LayerPicture*)la->GetImplementation();
        const uint64_t id = lp->GetGpuId();
        if (id == -1) return false;
        iLayerDrawInfo* me = (iLayerDrawInfo*)mLayerInfo.GetAddress(id);
        if (!me->mUploaded)
            return true;

        renderer->DestroyTexture(me->mTexture);
        renderer->DestroySampler(me->mSampler);

        me->mUploaded = false;
        return true;
    }

    bool LayerRendererPicture::LoadInGPU(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la)
    {
        LayerPicture* lp = (LayerPicture*)la->GetImplementation();
        const uint64_t id = lp->GetGpuId();
        iLayerDrawInfo* me = (iLayerDrawInfo*)mLayerInfo.GetAddress(id);

        return true;
    }

    static bool iUpload(piRenderer* renderer, iLayerDrawInfo* me, piLog* log)
    {
        piImage * image = me->mImage;

        piRenderer::Format format = piRenderer::Format::C1_8_UNORM;
        switch (image->GetFormat(0))
        {
        case piImage::FORMAT_I_GREY: format = piRenderer::Format::C1_8_UNORM; break;
        case piImage::FORMAT_I_RGBA: format = piRenderer::Format::C4_8_UNORM; break;
        default: return false;
        }

        piRenderer::TextureInfo info = { piRenderer::TextureType::T2D, format, me->mRes.x, me->mRes.y, me->mRes.z, 1 };

        void * pixelsBuffer = image->GetData(0);
        if (!pixelsBuffer)
        {
            log->Printf(LT_ERROR, L"Could not access local pixels.");
            return false;
        }

        switch (me->mType)
        {
        case LayerPicture::Image2D:
        case LayerPicture::Image360EquirectMono:
        case LayerPicture::Image360EquirectStereo:
        {
            me->mTexture = renderer->CreateTexture(0, &info, false, piRenderer::TextureFilter::MIPMAP, piRenderer::TextureWrap::REPEAT, 1.0f, pixelsBuffer);
            if (!me->mTexture)
                return false;

            me->mSampler = renderer->CreateSampler(piRenderer::TextureFilter::MIPMAP, piRenderer::TextureWrap::CLAMP, 1.0f);
            if (!me->mSampler)
                return false;

            break;
        }
        case LayerPicture::Image360CubemapCrossMono:
        {
            const unsigned int faceSize = me->mRes.x / 4;
            const unsigned int faceSizeSquared = faceSize * faceSize;
            const unsigned int nchannels = image->GetBpp(0);

            info.mType = piRenderer::TextureType::TCUBE;
            info.mXres = (int)faceSize;
            info.mYres = (int)faceSize;

            // We need to re-layout the cross into a vstrip -x,y,-y,y,-z,z
            void * cubemapLayoutBuffer = malloc(faceSizeSquared * 6 * nchannels);
            if (!cubemapLayoutBuffer)
            {
                log->Printf(LT_ERROR, L"Could not alloc cubemap layout buffer.");
                return false;
            }

            // We need to flipX faces 0,2,4,5 and flipY faces 1 and 3 ( in cross order)
            const bool flipX[6] = { true,true,false,false,true,true };
            const bool flipY[6] = { false,false,true,true,false,false };

            for (unsigned int face = 0; face < 6; face++)
            {
                ivec2 faceOffset; // offset in pixels to the cross face
                if (face == 1) faceOffset = ivec2(0, 1)*faceSize; //0 cross order
                if (face == 2) faceOffset = ivec2(1, 0)*faceSize; //1
                if (face == 5) faceOffset = ivec2(1, 1)*faceSize; //2
                if (face == 3) faceOffset = ivec2(1, 2)*faceSize; //3
                if (face == 0) faceOffset = ivec2(2, 1)*faceSize; //4
                if (face == 4) faceOffset = ivec2(3, 1)*faceSize; //5

                for (unsigned int y = 0; y < faceSize; y++)
                {
                    // flip Y or X
                    const unsigned int dstY = flipY[face] ? faceSizeSquared - (y + 1)*faceSize : y*faceSize + faceSize - 1;

                    if (flipX[face])
                    {
                        // we have to copy one pixel at a time to do the flip x
                        for (unsigned int x = 0; x < faceSize; x++)
                        {
                            unsigned char *src = (unsigned char*)pixelsBuffer + ((faceOffset.y + y)*me->mRes.x + faceOffset.x + x)*nchannels;
                            unsigned char *dst = (unsigned char*)cubemapLayoutBuffer + (face*faceSizeSquared + dstY - x)*nchannels;
                            memcpy(dst, src, nchannels);
                        }
                    }
                    else
                    {
                        // flip y can be done one scanline at a time
                        unsigned char *src = (unsigned char*)pixelsBuffer + ((faceOffset.y + y)*me->mRes.x + faceOffset.x)*nchannels;
                        unsigned char *dst = (unsigned char*)cubemapLayoutBuffer + (face*faceSizeSquared + dstY)*nchannels;
                        memcpy(dst, src, faceSize*nchannels);
                    }
                }
            }

            me->mTexture = renderer->CreateTexture(0, &info, false, piRenderer::TextureFilter::MIPMAP, piRenderer::TextureWrap::REPEAT, 1.0f, cubemapLayoutBuffer);
            if (!me->mTexture)
                return false;
            free(cubemapLayoutBuffer);
            break;
        }

        case LayerPicture::Image360CubemapVstripMono:
        {
            const int faceSize = me->mRes.x;
            info.mType = piRenderer::TextureType::TCUBE;
            info.mXres = faceSize;
            info.mYres = faceSize;
            me->mTexture = renderer->CreateTexture(0, &info, false, piRenderer::TextureFilter::MIPMAP, piRenderer::TextureWrap::REPEAT, 1.0f, pixelsBuffer);
            if (!me->mTexture)
                return false;
            break;
        }
        } // end switch

        me->mUploaded = true;


        return true;
    }

    void LayerRendererPicture::GlobalWork(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la, float masterVolume)
    {
    }

    void LayerRendererPicture::PrepareForDisplay(StereoMode stereoMode)
    {
        mVisibleLayerInfos.SetLength(0);
        mStereoMode = stereoMode;
    }

    void LayerRendererPicture::DisplayPreRender(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la, const frustum3& frus, const trans3d & layerToViewer, float laOpacity)
    {
        LayerPicture* lp = (LayerPicture*)la->GetImplementation();
        if (!la->GetLoaded()) return;

        uint32_t id = lp->GetGpuId();
        if (id == -1)
        {
            LoadInCPU(log, la);
        }
        id = lp->GetGpuId();
        piAssert(id != -1);


        iLayerDrawInfo* me = (iLayerDrawInfo*)mLayerInfo.GetAddress(id);

        const bound3 bbox = lp->GetBBox();

        // layer frustum culling
        if (boxInFrustum(frus, bbox) == 0)
        {
            //static int kk = 0; log->Printf(LT_MESSAGE, L"culled %s (%d)", la->GetName().GetS(), kk++);
            return;
        }

        // layer size culling (if too small in screen space)
        const vec3  lcen = getcenter(bbox);
        const vec3d vcen = (layerToViewer*f2d(vec4(lcen, 1.0f))).xyz();
        const float lrad2 = diagonalSquared(bbox);
        const double sc2 = lengthSquared((layerToViewer*vec4d(1.0, 0.0, 0.0, 0.0)).xyz());
        const double dis2 = lengthSquared(vcen);
        const double f = sqrt(double(lrad2) * sc2 / dis2);
        if (f < 0.005) // IQ-TODO: do a smooth fade here, super easy by using the layer opacity
        {
            return;
        }


        mVisibleLayerInfos.AppendUInt32(id, true);

        me->mType = lp->GetType();
        me->mFrontFaces = (layerToViewer.mFlip == flip3::N);
        me->mLayerState.setLayerToViewerInfo(layerToViewer);
        me->mLayerState.mOpacity = laOpacity;
        me->mLayerState.mID = la->GetID();
        #ifdef RENDER_BUDGET
        //me->mDistance = float(dis2);
        const vec3 lViewerPosition = d2f((invert(layerToViewer)*vec4d(0.0, 0.0, 0.0, 1.0)).xyz());
        me->mDistance = float(double(sdBox(lViewerPosition - lcen, getradiius(bbox))) * layerToViewer.mScale); // distance to closest point on the surface of the bbox. It's negative if we are inside.
        #endif
    }

    void LayerRendererPicture::DisplayRender(piRenderer* renderer, piLog* log, piBuffer layerStateShaderConstans, int capDelta)
    {
        const uint64_t num = mVisibleLayerInfos.GetLength();
        if (num < 1)
            return;

        if (renderer->GetAPI() == piRenderer::API::GL || renderer->GetAPI() == piRenderer::API::GLES)
            renderer->SetState(piSTATE_CULL_FACE, false);
        else
            renderer->SetRasterState(mRasterState);

        // Android uses the GL_OVR_multiview extension to render all layers of a 2D texture array.
        const int numInstances = (mStereoMode == StereoMode::Preferred && renderer->GetAPI() != piRenderer::API::GLES) ? 2 : 1;


        #ifdef RENDER_BUDGET
        {
            const piArray* data = (const piArray*)&mLayerInfo;
            std::sort((uint32_t*)mVisibleLayerInfos.GetAddress(0),
                (uint32_t*)mVisibleLayerInfos.GetAddress(num - 1),
                [data](int ida, int idb) -> bool
            {
                iLayerDrawInfo* meA = (iLayerDrawInfo*)data->GetAddress(ida);
                iLayerDrawInfo* meB = (iLayerDrawInfo*)data->GetAddress(idb);
                return meA->mDistance < meB->mDistance;
            });
        }
        #endif

        bool useBuffer = renderer->GetAPI() == piRenderer::API::DX;


        renderer->AttachShaderConstants(mShaderConstants, 9);


        #ifdef NVIDIA_BUG_WORKAROUND
        bool  cachedViewportDone = false;
        float cachedViewportData [2*6];
        #endif

        for (uint64_t j = 0; j < num; j++)
        {
            const uint32_t id = mVisibleLayerInfos.GetUInt32(j);
            iLayerDrawInfo* me = (iLayerDrawInfo*)mLayerInfo.GetAddress(id);

            renderer->UpdateBuffer(layerStateShaderConstans, &me->mLayerState, 0, sizeof(LayersState));


            const float data[4] = { me->mAspectRatio, 1.0f, 0.0f, 0.0f };

            int shaderID = static_cast<int>(mStereoMode);
#if defined(ANDROID)
            //shaderID += 3 * (mDebugRenderMode == Disabled ? 0 : mDebugRenderMode);
#endif

            renderer->SetState(piSTATE_FRONT_FACE, me->mFrontFaces);

            if (!me->mUploaded)
            {
                #ifdef NVIDIA_BUG_WORKAROUND
                if( mStereoMode==StereoMode::Preferred && !cachedViewportDone )
                {
                    int nn = 0;
                    renderer->GetViewports(&nn,cachedViewportData);
                    cachedViewportDone = true;
                }
                #endif

                if (!iUpload(renderer, me, log))
                    continue;
                #ifdef NVIDIA_BUG_WORKAROUND
                if( mStereoMode==StereoMode::Preferred  )
                {
                    renderer->SetViewports(2,cachedViewportData);
                }
                #endif
            }


            switch (me->mType)
            {
            case LayerPicture::Image2D:
                renderer->AttachShader(mShaders[shaderID][LayerPicture::Image2D]);
                renderer->AttachTextures(1, me->mTexture);
                renderer->AttachTextures(1, &mBlueNoise, 7);
                renderer->AttachSamplers(1, me->mSampler);

                if( !useBuffer )
                renderer->SetShaderConstant4F(0, (float*)data, 1);
                else
                {
                renderer->UpdateBuffer(mShaderConstants, (const void*)data, 0, 4*sizeof(float));
                }


                renderer->DrawUnitQuad_XY(numInstances);
                renderer->DettachTextures();
                renderer->DettachShader();
                break;
            case LayerPicture::Image360EquirectMono:
                renderer->AttachShader(mShaders[shaderID][LayerPicture::Image360EquirectMono]);
                renderer->AttachTextures(1, me->mTexture);
                renderer->AttachTextures(1, &mBlueNoise, 7);
                renderer->AttachSamplers(1, me->mSampler);
                m360SphereRenderMesh.Render(renderer, 0, 0, numInstances);
                renderer->DettachTextures();
                renderer->DettachShader();
                break;
            case LayerPicture::Image360EquirectStereo:
                renderer->AttachShader(mShaders[shaderID][LayerPicture::Image360EquirectStereo]);
                renderer->AttachTextures(1, me->mTexture);
                m360SphereRenderMesh.Render(renderer, 0, 0, numInstances);
                renderer->DettachTextures();
                renderer->DettachShader();
                break;
            case LayerPicture::Image360CubemapCrossMono:
            case LayerPicture::Image360CubemapVstripMono:
                renderer->AttachShader(mShaders[shaderID][LayerPicture::Image360CubemapCrossMono]);
                renderer->AttachTextures(1, me->mTexture);
                m360CubemapRenderMesh.Render(renderer, 0, 0, numInstances);
                renderer->DettachTextures();
                renderer->DettachShader();
                break;
            }
        }
        renderer->SetState(piSTATE_CULL_FACE, true);
        renderer->SetState(piSTATE_FRONT_FACE, true);
    }

}
