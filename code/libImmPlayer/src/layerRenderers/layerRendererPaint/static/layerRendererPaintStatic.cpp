#define CUSTOM_ALPHA_TO_COVERAGE 1


// IQ-TODO: check if Quest support glDrawIndirect, and if so, add it! 

#ifdef RENDER_BUDGET
#include <algorithm>
#endif

#include "libImmImporter/src/document/layerPaint/element.h"
#include "libImmImporter/src/document/layer.h"
#include "libImmImporter/src/document/layerPaint.h"
#include "libImmImporter/src/document/sequence.h"
#include "libImmImporter/src/document/layerPaint/drawingStatic.h"
#include "../../../blue_noise.h"


#include "layerRendererPaintStatic.h"

using namespace ImmCore;
using namespace ImmImporter;

namespace ImmPlayer
{
#if defined(WINDOWS)
    #include "tmp/shader_static_brush_vs_hlsl.inc"
    #include "tmp/shader_static_brush_fs_hlsl.inc"
    #include "shader_static_brush_vs.glsl"
    #include "shader_static_brush_fs.glsl"
#elif defined(ANDROID)
    #include "shader_static_brush_vs.es.glsl"
    #include "shader_static_brush_fs.es.glsl"
#endif


    typedef struct
    {
        uint32_t mVertexOffset;
        float    mBiggestStroke;
    }ChunkData;

    
    struct iSLayerDrawInfoStatic
    {
        struct BufferData
        {
            piBuffer		  mVertexData;
            piBuffer		  mIBO;
            piVertexArray	  mVertexArray[3];
            piTArray<const DrawingStatic::Geometry::Chunk *> mChunks;
        }mBuffers[LayerRendererPaintStatic::kNumChunkTypes];
        bool              mUploaded;

        //-----
        const DrawingStatic::Geometry *mGeometry;
        LayersState       mLayerState; // per camera pass
        bool              mDrawin;     // per camera pass
        bool              mWiggle;     // per camera pass
        #ifdef RENDER_BUDGET
        float        mDistance;
        #endif

        //-----

        bool Init( const Drawing *dr)
        {
            mUploaded = false;

            mGeometry = dynamic_cast<const DrawingStatic *> (dr)->GetGeometry();

            for (int chunkType = 0; chunkType < LayerRendererPaintStatic::kNumChunkTypes; chunkType++)
            {
                BufferData *buf = mBuffers + chunkType;

                //mBuffers[i].mChunkData = nullptr;
                buf->mVertexData = nullptr;
                buf->mIBO = nullptr;
                buf->mVertexArray[0] = nullptr;

                const uint32_t numChunks = dr->GetNumGeometryChunks(chunkType);

                if (!buf->mChunks.Init(numChunks, false))
                    return false;
            }

            return true;
        }

        void End(void)
        {
            for (int chunkType = 0; chunkType < LayerRendererPaintStatic::kNumChunkTypes; chunkType++)
            {
                mBuffers[chunkType].mChunks.End();
            }
        }

        void Download(piRenderer* renderer, piLog *log)
        {
            if (!mUploaded) return;

            for (int chunkType = 0; chunkType < LayerRendererPaintStatic::kNumChunkTypes; chunkType++)
            {
                if (mGeometry->mBuffers[chunkType].mPoints.GetLength()==0) continue;
                // GPU data

                if (renderer->GetAPI() == piRenderer::API::DX)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        renderer->DestroyVertexArray2(mBuffers[chunkType].mVertexArray[j]);
                    }
                }
                else
                {
                    renderer->DestroyVertexArray(mBuffers[chunkType].mVertexArray[0]);
                }

                renderer->DestroyBuffer(mBuffers[chunkType].mVertexData);
                renderer->DestroyBuffer(mBuffers[chunkType].mIBO);                
            }
            mUploaded = false;
        }


        bool Upload(piRenderer* renderer, piLog *log)
        {
            for (int chunkType = 0; chunkType < LayerRendererPaintStatic::kNumChunkTypes; chunkType++)
            {
                BufferData *dst = mBuffers + chunkType;
                const DrawingStatic::Geometry::Data *src = mGeometry->mBuffers + chunkType;
                if (src->mPoints.GetLength() == 0) continue;

                const uint32_t nv = static_cast<uint32_t>(src->mPoints.GetLength());
                const uint32_t ni = static_cast<uint32_t>(src->mIndices.GetLength());

                dst->mVertexData = renderer->CreateStructuredBuffer(src->mPoints.GetAddress(0), nv, sizeof(DrawingStatic::MyVertexFormat), piRenderer::BufferType::Dynamic, piRenderer::BufferUse::ShaderResource);
                if (dst->mVertexData == nullptr)
                {
                    log->Printf(LT_ERROR, L"Couldn't create data resource");
                    return false;
                }

                dst->mIBO = renderer->CreateBuffer(src->mIndices.GetAddress(0), ni * sizeof(uint16_t), piRenderer::BufferType::Dynamic, piRenderer::BufferUse::Index);
                if (dst->mIBO == nullptr)
                {
                    log->Printf(LT_ERROR, L"Couldn't create mIBO");
                    return false;
                }

                if (renderer->GetAPI() == piRenderer::API::DX)
                {
                    #if !defined(ANDROID)
                    for (int j = 0; j < 3; j++)
                    {
                        dst->mVertexArray[j] = renderer->CreateVertexArray2(0, nullptr, nullptr, nullptr, nullptr, shader_static_brush_vs_code[chunkType], shader_static_brush_vs_size[chunkType], dst->mIBO, piRenderer::IndexArrayFormat::UINT_16);
                        if (!dst->mVertexArray[j])
                        {
                            log->Printf(LT_ERROR, L"Couldn't create Vertex Array");
                            return false;
                        }
                    }
                    #endif
                }
                else
                {
                    dst->mVertexArray[0] = renderer->CreateVertexArray(0, nullptr, nullptr, nullptr, nullptr, dst->mIBO, piRenderer::IndexArrayFormat::UINT_16);
                    if (!dst->mVertexArray[0])
                    {
                        log->Printf(LT_ERROR, L"Couldn't create Vertex Array");
                        return false;
                    }
                }
            }
            mUploaded = true;
            return true;
        }

    };

    //===================================================================


    LayerRendererPaintStatic::LayerRendererPaintStatic() : LayerRendererPaint() {}
    LayerRendererPaintStatic::~LayerRendererPaintStatic() {}

    bool LayerRendererPaintStatic::Init(piRenderer* renderer, piLog* log, Drawing::ColorSpace colorSpace, bool frontIsCCW)
    {
        #if ST_VERTEX_FORMAT == 1
        piAssert( sizeof(DrawingStatic::MyVertexFormat)==28 );
        #else
        piAssert( sizeof(DrawingStatic::MyVertexFormat)==36 );
        #endif

        mCapLayersToRender = 1;

        mColorSpace = colorSpace;
        if (!mLayerInfo.Init(256, sizeof(iSLayerDrawInfoStatic))) // one per layer
            return false;

        if (!mVisibleLayerInfos.Init(256, sizeof(uint32_t)))
            return false;

        int dindex = 0;
        for (int l = 0; l < 5; l++) // brush
        for (int k = 0; k < 2; k++) // wiggle
        for (int j = 0; j < 2; j++) // drawin
        for (int i = 0; i < 3; i++) // stereo
        {
            if (renderer->GetAPI() == piRenderer::API::GL && static_cast<StereoMode>(i) == StereoMode::Preferred &&
                (!renderer->SupportsFeature(piRenderer::RendererFeature::VIEWPORT_ARRAY) ||
                    !renderer->SupportsFeature(piRenderer::RendererFeature::VERTEX_VIEWPORT)
                    )
                )
            {
                // skip compiling fast stereo shaders when we don't support the feature
                dindex++;
                continue;
            }
                
#if defined(ANDROID)
            if (j == 1) continue; // skip the drawin shaders on Android
#endif
            const piShaderOptions ops = { 6,{ { "COLOR_COMPRESSED", static_cast<int>(colorSpace) },
                                              { "BRUSHTYPE", l },
                                              { "WIGGLE", k },
                                              { "DRAWIN", j },
                                              #if ST_VERTEX_FORMAT == 1
                                              { "VERTEX_FORMAT", 1 },
                                              #else
                                              { "VERTEX_FORMAT", 0 },
                                              #endif
                                              { "STEREOMODE", i } } };

            char error[1024] = { 0 };


            if (renderer->GetAPI() == piRenderer::API::GL || renderer->GetAPI() == piRenderer::API::GLES)
            {
                mShader[dindex] = renderer->CreateShader(&ops, shader_static_brush_vs, nullptr, nullptr, nullptr, shader_static_brush_fs, error);
            }
            else
            {
#ifndef ANDROID
                int vs_index = i +
                    j * 3 +
                    k * 3 * 2 +
                    l * 3 * 2 * 2 +
                    (static_cast<int>(colorSpace)) * 5 * 3 * 2 * 2;
                const int fs_index = i;

                mShader[dindex] = renderer->CreateShaderBinary(nullptr, shader_static_brush_vs_code[vs_index], shader_static_brush_vs_size[vs_index], nullptr, 0, nullptr, 0, nullptr, 0,
                    shader_static_brush_fs_code[fs_index], shader_static_brush_fs_size[fs_index], error);
#endif
            }

            if (!mShader[dindex])
            {
                piString tmp; tmp.InitCopyS(error);
                log->Printf(LT_ERROR, L"Could not create shader (%d,%d,%d,%d): %s", i, j, k, l, tmp.GetS());
                tmp.End();
                return false;
            }
            dindex++;
        }

        mRasterState[0] = renderer->CreateRasterState(false, frontIsCCW, piRenderer::CullMode::NONE, true, false);  // double sided, flip NO
        mRasterState[1] = renderer->CreateRasterState(false, frontIsCCW, piRenderer::CullMode::BACK, true, false);  // single sided, flip NO
        mRasterState[2] = renderer->CreateRasterState(false,!frontIsCCW, piRenderer::CullMode::NONE, true, false);  // double sided, flip YES
        mRasterState[3] = renderer->CreateRasterState(false,!frontIsCCW, piRenderer::CullMode::FRONT, true, false); // single sided, flip YES

        if (!mRasterState[0]) return false;
        if (!mRasterState[1]) return false;
        if (!mRasterState[2]) return false;
        if (!mRasterState[3]) return false;

        mChunkData = renderer->CreateBuffer(nullptr, 128*sizeof(ChunkData), piRenderer::BufferType::Dynamic, piRenderer::BufferUse::Constant);
        if (!mChunkData)
            return false;

        const piRenderer::TextureInfo infob = { piRenderer::TextureType::T2D_ARRAY, piRenderer::Format::C1_8_UNORM, 64, 64, 64, 1 };
        mBlueNoise = renderer->CreateTexture(0, &infob, false, piRenderer::TextureFilter::NONE, piRenderer::TextureWrap::REPEAT, 1.0f, (void*)GetBlueNoise_64x64x64());

        return true;
    }

    void LayerRendererPaintStatic::Deinit(piRenderer* renderer, piLog* log)
    {
        // Verify everything is freed for memory leaks
        const uint64_t num = mLayerInfo.GetMaxLength();
        bool notDeleted = false;
        bool stillUploaded = false;
        for (uint64_t j = 0; j < num; j++)
        {
            iSLayerDrawInfoStatic* me = (iSLayerDrawInfoStatic*)mLayerInfo.GetAddress(j);
            if (mLayerInfo.IsUsed(j))
            {
                notDeleted = true;
                if (me->mUploaded)            stillUploaded = true;
            }
        }
        piAssert(notDeleted == false);
        piAssert(stillUploaded == false);

        for (int i = 0; i < kNumShaders; i++)
        {
            renderer->DestroyShader(mShader[i]);
        }

        renderer->DestroyRasterState(mRasterState[3]);
        renderer->DestroyRasterState(mRasterState[2]);
        renderer->DestroyRasterState(mRasterState[0]);
        renderer->DestroyRasterState(mRasterState[1]);
        renderer->DestroyBuffer(mChunkData);
        renderer->DestroyTexture(mBlueNoise);

        mVisibleLayerInfos.End();
        mLayerInfo.End();
    }

    void LayerRendererPaintStatic::UnloadInCPU(piLog* log, Layer* la)
    {
        LayerPaint* lp = (LayerPaint*)la->GetImplementation();

        const int numDrawings = lp->GetNumDrawings();

        for (int j = 0; j < numDrawings; j++)
        {
            const Drawing *dr = lp->GetDrawing(j);
            const int id = dr->GetGpuId();
            if (id == -1) return;
            iSLayerDrawInfoStatic* me = (iSLayerDrawInfoStatic*)mLayerInfo.GetAddress(id);            
            me->End();
            mLayerInfo.Free(id);
        }
    }

    bool LayerRendererPaintStatic::IsLoadedInGPU(Layer * la)
    {
        LayerPaint* lp = (LayerPaint*)la->GetImplementation();
        const int numDrawings = lp->GetNumDrawings();

        for (int j = 0; j < numDrawings; j++)
        {
            Drawing *dr = lp->GetDrawing(j);
            const int id = dr->GetGpuId();
            if (id == -1) return false;
            iSLayerDrawInfoStatic* me = (iSLayerDrawInfoStatic*)mLayerInfo.GetAddress(id);
            if (me->mUploaded) 
                return true;
        }
        return false;
    }   

    bool LayerRendererPaintStatic::UnloadInGPU(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la)
    {
        LayerPaint* lp = (LayerPaint*)la->GetImplementation();

        const int numDrawings = lp->GetNumDrawings();

        for (int j = 0; j < numDrawings; j++)
        {
            Drawing *dr = lp->GetDrawing(j);
            const int id = dr->GetGpuId();
            if (id == -1) continue;
            iSLayerDrawInfoStatic* me = (iSLayerDrawInfoStatic*)mLayerInfo.GetAddress(id);
            me->Download(renderer, log);
        }

        return true;
    }

    bool LayerRendererPaintStatic::UnloadInGPU(ImmCore::piRenderer * renderer, ImmCore::piSoundEngine * sound, ImmCore::piLog * log, Layer * la, unsigned int drawingID)
    {
        LayerPaint* lp = (LayerPaint*)la->GetImplementation();
        Drawing *dr = lp->GetDrawing(drawingID);
        const int id = dr->GetGpuId();
        if (id == -1) return false;
        iSLayerDrawInfoStatic* me = (iSLayerDrawInfoStatic*)mLayerInfo.GetAddress(id);
        me->Download(renderer, log);
        return true;
    }

    bool LayerRendererPaintStatic::LoadInCPU( piLog* log, Layer* la)
    {
        LayerPaint* lp = (LayerPaint*)la->GetImplementation();

        const int numDrawings = lp->GetNumDrawings();

        int s = sizeof(iSLayerDrawInfoStatic);
        for (int j = 0; j < numDrawings; j++)
        {
            bool isNew = false;
            uint64_t id;
            iSLayerDrawInfoStatic* me = (iSLayerDrawInfoStatic*)mLayerInfo.Alloc(&isNew, &id, true);
            if (!me)
            {
                log->Printf(LT_ERROR, L"Couldn't alloc new DrawInfo");
                return false;
            }
            new (me) iSLayerDrawInfoStatic();

            Drawing* dr = lp->GetDrawing(j);
            if (!me->Init( dr ))
            {
                log->Printf(LT_ERROR, L"Couldn't Init DrawInfo");
                return false;
            }
            dr->SetGpuId(static_cast<int>(id));
        }
        return true;
    }

    bool LayerRendererPaintStatic::LoadInGPU(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la)
    {
        return true;
        /*
        LayerPaint* lp = (LayerPaint*)la->GetImplementation();

        const int numDrawings = lp->GetNumDrawings();

        for (int j = 0; j < numDrawings; j++)
        {
            const uint32_t id = lp->GetGpuId(j);
            iSLayerDrawInfo* me = (iSLayerDrawInfo*)mLayerInfo.GetAddress(id);

            if (!me->Upload(renderer, log))
            {
                log->Printf(LT_ERROR, L"Couldn't Init DrawInfo");
                return false;
            }
        }

        return true;
        */
    }


    void LayerRendererPaintStatic::GlobalWork(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la, float masterVolume)
    {

    }

    void LayerRendererPaintStatic::PrepareForDisplay(StereoMode stereoMode)
    {
        mStereoMode = stereoMode;
        mVisibleLayerInfos.SetLength(0);
    }

    void LayerRendererPaintStatic::DisplayPreRender(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la, const frustum3& frus, const trans3d & layerToViewer, float laOpacity)
    {
        LayerPaint* lp = (LayerPaint*)la->GetImplementation();
        if (!la->GetLoaded()) return;

        const Drawing *dr = lp->GetCurrentDrawing();

        if (!dr->GetLoaded())
            return;

        const bound3 bbox = dr->GetBBox();//lp->GetBBox(drawing);

        mDrawCallInfo.numDrawCallsCulled = 0;
        mDrawCallInfo.numTrianglesCulled = 0;

        uint32_t id = dr->GetGpuId();
        if (id == -1)
        {
            LoadInCPU(log, la);
        }
        id = dr->GetGpuId();
        piAssert(id != -1);

        // layer frustum culling
        if (boxInFrustum(frus, bbox) == 0)
        {
            
            iSLayerDrawInfoStatic* me = (iSLayerDrawInfoStatic*)mLayerInfo.GetAddress(id);

            for (uint32_t chunkType = 0; chunkType < kNumChunkTypes; chunkType++)
            {
                if (me->mGeometry->mBuffers[chunkType].mPoints.GetLength() == 0) continue;

                const uint64_t numChunks = me->mGeometry->mBuffers[chunkType].mChunks.GetLength();
                mDrawCallInfo.numDrawCallsCulled += static_cast<int>(numChunks);

                for (uint64_t i = 0; i < numChunks; i++)
                {
                    const DrawingStatic::Geometry::Chunk *srcChunk = me->mGeometry->mBuffers[chunkType].mChunks.GetAddress(i);
                    mDrawCallInfo.numTrianglesCulled += srcChunk->mNumIndices;
                }
            }

            //static int kk = 0; log->Printf(LT_MESSAGE, L"culled %s (%d)", la->GetName().GetS(), kk++);
            return;
        }


        // layer size culling (if too small in screen space)
        const vec3  lcen = getcenter(bbox);
        const vec3  lViewerPosition = d2f((invert(layerToViewer)*vec4d(0.0, 0.0, 0.0, 1.0)).xyz());
        const float wDistanceToBBox = float(double(sdBox(lViewerPosition - lcen, getradiius(bbox))) * layerToViewer.mScale); // distance to closest point on the surface of the bbox. It's negative if we are inside.
        if (wDistanceToBBox > 0.0) // if outside bbox
        {
            const vec3d vcen = (layerToViewer*f2d(vec4(lcen, 1.0f))).xyz();
            const float lrad2 = diagonalSquared(bbox);
            const double dis2 = lengthSquared(vcen);
            const double sizeInScreen = layerToViewer.mScale * sqrt(double(lrad2) / dis2);
            if (sizeInScreen < 0.005) // IQ-TODO: do a smooth fade here, super easy by using the layer opacity
            {
                return;
            }
        }
        //---------
        
        iSLayerDrawInfoStatic* me = (iSLayerDrawInfoStatic*)mLayerInfo.GetAddress(id);

        // chunk frustum culling
        bool anyVisible = false;
        for (uint32_t chunkType = 0; chunkType < kNumChunkTypes; chunkType++)
        {
            me->mBuffers[chunkType].mChunks.SetLength(0);
            if (me->mGeometry->mBuffers[chunkType].mPoints.GetLength() == 0) continue;

            const uint64_t numChunks = me->mGeometry->mBuffers[chunkType].mChunks.GetLength();
            for (uint64_t i = 0; i < numChunks; i++)
            {
                const DrawingStatic::Geometry::Chunk *srcChunk = me->mGeometry->mBuffers[chunkType].mChunks.GetAddress(i);
                const bool visible = boxInFrustum(frus, srcChunk->mBBox) != 0;
                if (!visible)
                {
                    ++mDrawCallInfo.numDrawCallsCulled;
                    mDrawCallInfo.numTrianglesCulled += srcChunk->mNumIndices;
                    continue;
                }

                me->mBuffers[chunkType].mChunks.Append(srcChunk, true);
                anyVisible = true;
            }
        }

        if (!anyVisible) return;

        // at this point, some content in this layer is visible. Mark it as such
        mVisibleLayerInfos.AppendUInt32(id, true);


        // prepare GPU data
        {
            me->mLayerState.setLayerToViewerInfo(layerToViewer);
            me->mLayerState.mOpacity = laOpacity;
            //me->mLayerState.mFlipSign = (la->GetTransformToWorld().mFlip == flip3::N) ? 1.0f : -1.0f;
            me->mLayerState.mDrawInTime = float(la->GetDrawInTime());
            me->mLayerState.mAnimParam.x = float(la->GetAnimParam(0));
            me->mLayerState.mAnimParam.y = float(la->GetAnimParam(1));
            me->mLayerState.mAnimParam.z = float(la->GetAnimParam(2));
            me->mLayerState.mAnimParam.w = float(la->GetAnimParam(3));
            me->mLayerState.mID = la->GetID();

            const KeepAlive * ka = la->GetKeepAlive();

            if (ka->GetType() == KeepAlive::KeepAliveType::Wiggle)
            {
                const KeepAlive::Wiggle *fx = ka->GetDataWiggle();
                me->mLayerState.mKeepAlive.mWiggle.mAmplitude = fx->mAmplitude;
                me->mLayerState.mKeepAlive.mWiggle.mFrequency = fx->mFrequency;
                me->mLayerState.mKeepAlive.mWiggle.mSpeed = fx->mSpeed;
            }
            else if (ka->GetType() == KeepAlive::KeepAliveType::Blink)
            {
                const KeepAlive::Blink *fx = ka->GetDataBlink();
                me->mLayerState.mKeepAlive.mBlink.mWaveForm = fx->mWaveForm;
                me->mLayerState.mKeepAlive.mBlink.mSpeed = fx->mSpeed;
                me->mLayerState.mKeepAlive.mBlink.mMinOut = fx->mMinOut;
                me->mLayerState.mKeepAlive.mBlink.mMaxOut = fx->mMaxOut;
                me->mLayerState.mKeepAlive.mBlink.mMinIn = fx->mMinIn;
                me->mLayerState.mKeepAlive.mBlink.mMaxIn = fx->mMaxIn;
            }

            me->mDrawin = la->GetLayerUsesDrawin();
            me->mWiggle = ka->GetType() == KeepAlive::KeepAliveType::Wiggle;
            #ifdef RENDER_BUDGET
            //me->mDistance = (wDistanceToBBox<0.0) ? wDistanceToBBox : 1.0f/sizeInScreen;
            //me->mDistance = wDistanceToBBox;
            me->mDistance = float(1.0 / sizeInScreen);

            //me->mDistance = float(dis2);
            #endif
        }
    }

    void LayerRendererPaintStatic::DisplayRender(piRenderer* renderer, piLog* log, piBuffer layerStateShaderConstans, int capDelta)
    {
        const uint64_t num = mVisibleLayerInfos.GetLength();
        if (num < 1) return;


        int lastShaderID = -1;
        int lastStateID = -1;

        const int stereoModeInt = static_cast<int>(mStereoMode);

        // Android uses the GL_OVR_multiview extension to render all layers of a 2D texture array.
        const int numInstances = (mStereoMode == StereoMode::Preferred && renderer->GetAPI() != piRenderer::API::GLES) ? 2 : 1;

        renderer->AttachShaderConstants(mChunkData, 9);


        #ifdef RENDER_BUDGET
        {
            const iSLayerDrawInfoStatic* data = (const iSLayerDrawInfoStatic*)mLayerInfo.GetAddress(0);
            std::sort((uint32_t*)mVisibleLayerInfos.GetAddress(0), (uint32_t*)mVisibleLayerInfos.GetAddress(num - 1),
                [data](int ida, int idb) -> bool
                {
                    return data[ida].mDistance < data[idb].mDistance;
                });
        }

        mCapLayersToRender = static_cast<uint64_t>(clamp( int(mCapLayersToRender) + capDelta, 1, int(num)));
        //log->Printf(LT_MESSAGE, L"%d", mCapLayersToRender);

        const uint64_t numToRender = (num > mCapLayersToRender) ? mCapLayersToRender : num;

        #else
        const uint64_t numToRender = num;
        #endif

        mDrawCallInfo.numDrawCalls = 0;
        mDrawCallInfo.numTriangles = 0;

        // for each visible layer
        for (uint64_t j = 0; j < numToRender; j++)
        {
            const uint32_t id = mVisibleLayerInfos.GetUInt32(j);
            iSLayerDrawInfoStatic* me = (iSLayerDrawInfoStatic*)mLayerInfo.GetAddress(id);

            // upload to GPU, if needed. This should be commaded externally as we stream scenes. But for now we do it here
            if (!me->mUploaded)
            {
                if (!me->Upload(renderer, log))
                {
                    log->Printf(LT_ERROR, L"Couldn't upload data to the GPU");
                    return;
                }
            }

            renderer->UpdateBuffer(layerStateShaderConstans, &me->mLayerState, 0, sizeof(LayersState));

            int tmpShaderId = 0;
            tmpShaderId += stereoModeInt;
#if !defined(ANDROID)
            tmpShaderId += (me->mDrawin == true) ? 3 : 0;
#endif
            tmpShaderId += (me->mWiggle == true) ? 6 : 0;

            renderer->AttachTextures(1, &mBlueNoise, 7);
            
            // for each chunk in that layer
            for (int chunkType = 0; chunkType < kNumChunkTypes; chunkType++)
            {
                const iSLayerDrawInfoStatic::BufferData *info = me->mBuffers + chunkType;
                const uint64_t numChunks = info->mChunks.GetLength();
                if (numChunks == 0) continue;

                // set shader and state
                const int stateID = ((chunkType == static_cast<int>(Element::BrushSectionType::Segment)) ? 0 : 1);                                    
                if (stateID != lastStateID) { lastStateID = stateID; renderer->SetRasterState(mRasterState[stateID]); }

#if !defined(ANDROID)
                int shaderID = tmpShaderId + 2 * 2 * 3 * chunkType;
#else
                int shaderID = tmpShaderId + 2 * 3 * chunkType;
#endif
                if (shaderID != lastShaderID) { lastShaderID = shaderID; renderer->AttachShader(mShader[shaderID]); }


                // attach vertex and index data
                renderer->AttachShaderBuffer(me->mBuffers[chunkType].mVertexData, 8);
                if (renderer->GetAPI() == piRenderer::API::GL || renderer->GetAPI() == piRenderer::API::GLES)
                    renderer->AttachVertexArray(me->mBuffers[chunkType].mVertexArray[0]);
                else
                    renderer->AttachVertexArray2(me->mBuffers[chunkType].mVertexArray[stereoModeInt]);

                mDrawCallInfo.numDrawCalls += static_cast<int>(numChunks);

                // TODO: all this loop below could be implemented with a single render call! (piRenderer::DrawPrimitiveIndexedMultiple)
                for (uint64_t i = 0; i < numChunks; i++)
                {
                    const DrawingStatic::Geometry::Chunk *chunk = info->mChunks.Get(i);
                    const ChunkData cd = { chunk->mVertexOffset, chunk->mBiggestStroke };

                    renderer->UpdateBuffer(mChunkData, &cd, 0, 1 * sizeof(ChunkData)); // TODO:  put this in an buffer so we only do one upload per chunkType, not per chunk

                    renderer->DrawPrimitiveIndexed(piRenderer::PrimitiveType::TriangleStrip, chunk->mNumIndices, numInstances, 0 /*chunk->mVertexOffset*/, 0, chunk->mIndexOffset);
                    mDrawCallInfo.numIndices += chunk->mNumIndices;
                    mDrawCallInfo.numTriangles += chunk->mNumPolygons * 2;
                }
            }
        }

        //log->Printf(LT_DEBUG, L"Total number of indices in viewport: %i", totalNumIndices);
        renderer->DettachTextures();
        renderer->DettachShader();
    }

}
