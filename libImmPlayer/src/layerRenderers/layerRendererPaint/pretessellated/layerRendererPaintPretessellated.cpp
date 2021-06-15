// TODO: use drawIndirect so we can render all chunk of the same type at once
// TODO; can reduce index buffer switches IF we put all vertices of all brush types together --> need to change index buffers to 
//       32 bit or need to user vertexBase -- I think it doesn't work in DX?
// TODO: merge all chuns of all layers together. Need a buffer for LayerState
// TODO: do 16 bit vertices, upload bbox corner to ChunkData buffer. CUrrently 20 bytes, could become 14 bytes
// TODO: port to DX (shader mostly)


#define CUSTOM_ALPHA_TO_COVERAGE 1

#include "libCore/src/libBasics/piArray.h"
#include "libCore/src/libBasics/piPool.h"
#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libRender/piRenderer.h"
#include "libCore/src/libSound/piSound.h"
#include "libCore/src/libBasics/piFile.h"
#include "libCore/src/libBasics/piStr.h"
#include "libCore/src/libBasics/piDebug.h"

#include "libImmImporter/src/document/layerPaint/element.h"
#include "libImmImporter/src/document/layer.h"
#include "libImmImporter/src/document/layerPaint.h"
#include "libImmImporter/src/document/sequence.h"
#include "libImmImporter/src/document/layerPaint/drawingPretessellated.h"
#include "../../../blue_noise.h"

#include "layerRendererPaintPretessellated.h"

using namespace ImmCore;
using namespace ImmImporter;

namespace ImmPlayer
{
#if defined(ANDROID)
	#include "tmp/shader_pretessellated_brush_GLES_glsl.h"
#else
	#include "tmp/shader_pretessellated_brush_vs_hlsl.inc"
	#include "tmp/shader_pretessellated_brush_fs_hlsl.inc"
    #include "shader_pretessellated_brush_vs.glsl"
    #include "shader_pretessellated_brush_fs.glsl"
#endif

	typedef struct
	{
		uint32_t dummy;
	}ChunkData;

	
	struct iSLayerDrawInfoPretessellated
	{
		struct BufferData
		{
			piBuffer		  mVBO;
			piBuffer		  mIBO;
			piVertexArray	  mVertexArray[3];
			piTArray<const DrawingPretessellated::Geometry::Chunk*> mChunks;
		}mBuffers[LayerRendererPaintPretessellated::kNumChunkTypes];
        
        const DrawingPretessellated::Geometry *mGeometry;
        bool              mUploaded;
		LayersState       mLayerState; // per camera pass
		bool              mDrawin;     // per camera pass
		bool              mWiggle;     // per camera pass


		//-----

		bool Init( const Drawing *dr)
		{

			mUploaded = false;
			mGeometry = dynamic_cast<const DrawingPretessellated *>(dr)->GetGeometry();

			for (int chunkType = 0; chunkType < LayerRendererPaintPretessellated::kNumChunkTypes; chunkType++)
			{
				BufferData *buf = mBuffers + chunkType;

				//mBuffers[i].mChunkData = nullptr;
				buf->mVBO = nullptr;
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
			for (int chunkType = 0; chunkType < LayerRendererPaintPretessellated::kNumChunkTypes; chunkType++)
			{
				mBuffers[chunkType].mChunks.End();
			}
		}

		void Download(piRenderer* renderer)
		{
			if (!mUploaded) return;

			for (int chunkType = 0; chunkType < LayerRendererPaintPretessellated::kNumChunkTypes; chunkType++)
			{
				if (mGeometry->mBuffers[chunkType].mVertices.GetLength()==0) continue;

                if (renderer->GetAPI() == piRenderer::API::DX)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        renderer->DestroyVertexArray(mBuffers[chunkType].mVertexArray[j]);
                    }
                }
                else
                {
                    renderer->DestroyVertexArray(mBuffers[chunkType].mVertexArray[0]);
                }

				// GPU data
				renderer->DestroyBuffer(mBuffers[chunkType].mVBO);
				renderer->DestroyBuffer(mBuffers[chunkType].mIBO);			
			}
			mUploaded=false;
		}


		bool Upload(piRenderer* renderer, piLog *log)
		{
			for (int chunkType = 0; chunkType < LayerRendererPaintPretessellated::kNumChunkTypes; chunkType++)
			{
				BufferData *dst = mBuffers + chunkType;
				const DrawingPretessellated::Geometry::Data *src = mGeometry->mBuffers + chunkType;
                const uint32_t nv = static_cast<uint32_t>(src->mVertices.GetLength());
                const uint32_t ni = static_cast<uint32_t>(src->mIndices.GetLength());
                if (nv == 0) continue;


				dst->mVBO = renderer->CreateBuffer(src->mVertices.GetAddress(0), nv*sizeof(DrawingPretessellated::MyVertexFormat), piRenderer::BufferType::Dynamic, piRenderer::BufferUse::Vertex);
				if (dst->mVBO == nullptr)
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
#ifndef ANDROID
					for (int j = 0; j < 3; j++)
					{
						dst->mVertexArray[j] = renderer->CreateVertexArray2(0, nullptr, nullptr, nullptr, nullptr, shader_pretessellated_brush_vs_code[chunkType], shader_pretessellated_brush_vs_size[chunkType], dst->mIBO, piRenderer::IndexArrayFormat::UINT_16);
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

#if PT_VERTEX_FORMAT==1
					const piRArrayLayout vf = { 20, 4, 0,{
							{ 3, piRArrayDataType::piRArrayType_Float, false },
							{ 4, piRArrayDataType::piRArrayType_UByte, true },
							{ 3, piRArrayDataType::piRArrayType_UByte, false },
							{ 1, piRArrayDataType::piRArrayType_UByte, false },
							 } };
#else
					const piRArrayLayout vf = { 24, 5, 0,{
							{ 3, piRArrayDataType::piRArrayType_Float, false },
							{ 4, piRArrayDataType::piRArrayType_UByte, true },
							{ 3, piRArrayDataType::piRArrayType_UByte, false },
							{ 1, piRArrayDataType::piRArrayType_UByte, false },
							{ 1, piRArrayDataType::piRArrayType_Float, false } } };
#endif

				
					dst->mVertexArray[0] = renderer->CreateVertexArray(1, dst->mVBO, &vf, nullptr, nullptr, dst->mIBO, piRenderer::IndexArrayFormat::UINT_16);
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


	LayerRendererPaintPretessellated::LayerRendererPaintPretessellated() : LayerRendererPaint() {}
	LayerRendererPaintPretessellated::~LayerRendererPaintPretessellated() {}

	bool LayerRendererPaintPretessellated::Init(piRenderer* renderer, piLog* log, Drawing::ColorSpace colorSpace, bool frontIsCCW)
	{
		//	piAssert(sizeof(MyVertexFormat) == 40);

		mColorSpace = colorSpace;

        if (!mLayerInfo.Init(128, sizeof(iSLayerDrawInfoPretessellated))) // one per layer
			return false;

		if (!mVisibleLayerInfos.Init(128, sizeof(uint32_t)))
			return false;

		int dindex = 0;

        for (int k = 0; k < 2; k++) // wiggle
		for (int j = 0; j < 2; j++) // drawin
		for (int i = 0; i < 3; i++) // stereo
		{
            if (renderer->GetAPI()==piRenderer::API::GL && static_cast<StereoMode>(i) == StereoMode::Preferred &&
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
		    if (j >= 1) continue; // skip drawin shaders
#endif
			const piShaderOptions ops = { 5,{ { "COLOR_COMPRESSED", static_cast<int>(colorSpace) },
												{ "WIGGLE", k },
												{ "DRAWIN", j },
#if PT_VERTEX_FORMAT==1
											  	{ "VERTEX_FORMAT", 1 },
#else
											  	{ "VERTEX_FORMAT", 0 },
#endif
												{ "STEREOMODE", static_cast<int>(i) } } };


			char error[1024] = { 0 };


			if (renderer->GetAPI() == piRenderer::API::GL || renderer->GetAPI() == piRenderer::API::GLES)
			{
				mShader[dindex] = renderer->CreateShader(&ops, shader_pretessellated_brush_vs, nullptr, nullptr, nullptr, shader_pretessellated_brush_fs, error);
			}
			else
			{
                #ifndef ANDROID
				int vs_index = i +
					j * 3 +
					k * 3 * 2 +
					(static_cast<int>(colorSpace)) * 3 * 2 * 2;
				const int fs_index = i;

				mShader[dindex] = renderer->CreateShaderBinary(nullptr, shader_pretessellated_brush_vs_code[vs_index], shader_pretessellated_brush_vs_size[vs_index], nullptr, 0, nullptr, 0, nullptr, 0,
					shader_pretessellated_brush_fs_code[fs_index], shader_pretessellated_brush_fs_size[fs_index], error);
                #endif
			}

			if (!mShader[dindex])
			{
				piString tmp; tmp.InitCopyS(error);
				log->Printf(LT_ERROR, L"Could not create shader (%d,%d,%d): %s", i, j, k, tmp.GetS());
				tmp.End();
				return false;
			}
			dindex++;
		}

        mRasterState[0] = renderer->CreateRasterState(false, frontIsCCW, piRenderer::CullMode::NONE, true, false);  // double sided, flip NO
        mRasterState[1] = renderer->CreateRasterState(false, frontIsCCW, piRenderer::CullMode::BACK, true, false);  // single sided, flip NO
        mRasterState[2] = renderer->CreateRasterState(false, frontIsCCW, piRenderer::CullMode::NONE, true, false);  // double sided, flip YES
        mRasterState[3] = renderer->CreateRasterState(false, frontIsCCW, piRenderer::CullMode::FRONT, true, false); // single sided, flip YES

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

	void LayerRendererPaintPretessellated::Deinit(piRenderer* renderer, piLog* log)
	{
        // Verify everything is freed for memory leaks
        const uint64_t num = mLayerInfo.GetMaxLength();
        bool notDeleted = false;
        bool stillUploaded = false;
        for (uint64_t j = 0; j < num; j++)
        {
            iSLayerDrawInfoPretessellated* me = (iSLayerDrawInfoPretessellated*)mLayerInfo.GetAddress(j);
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

        renderer->DestroyTexture(mBlueNoise);

		renderer->DestroyRasterState(mRasterState[3]);
		renderer->DestroyRasterState(mRasterState[2]);
		renderer->DestroyRasterState(mRasterState[0]);
		renderer->DestroyRasterState(mRasterState[1]);
		
        renderer->DestroyBuffer(mChunkData);

		mVisibleLayerInfos.End();
		mLayerInfo.End();
	}

	void LayerRendererPaintPretessellated::UnloadInCPU(piLog* log, Layer* la)
	{
		LayerPaint* lp = (LayerPaint*)la->GetImplementation();

		const int numDrawings = lp->GetNumDrawings();

		for (int j = 0; j < numDrawings; j++)
		{
			const Drawing *dr = lp->GetDrawing(j);
			const int id = dr->GetGpuId();
            if (id == -1) return;
			iSLayerDrawInfoPretessellated* me = (iSLayerDrawInfoPretessellated*)mLayerInfo.GetAddress(id);
			me->End();
			mLayerInfo.Free(id);
		}
	}

    bool LayerRendererPaintPretessellated::IsLoadedInGPU(Layer * la)
    {
        LayerPaint* lp = (LayerPaint*)la->GetImplementation();
        const int numDrawings = lp->GetNumDrawings();

        for (int j = 0; j < numDrawings; j++)
        {
            Drawing *dr = lp->GetDrawing(j);
            const int id = dr->GetGpuId();
            if (id == -1) return false;
            iSLayerDrawInfoPretessellated* me = (iSLayerDrawInfoPretessellated*)mLayerInfo.GetAddress(id);
            if (me->mUploaded) return true;
        }
        return false;
    }


	bool LayerRendererPaintPretessellated::UnloadInGPU(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la)
	{
		LayerPaint* lp = (LayerPaint*)la->GetImplementation();

		const int numDrawings = lp->GetNumDrawings();

		for (int j = 0; j < numDrawings; j++)
		{
			const Drawing *dr = lp->GetDrawing(j);
			const int id = dr->GetGpuId();
            if (id == -1) return false;
			iSLayerDrawInfoPretessellated* me = (iSLayerDrawInfoPretessellated*)mLayerInfo.GetAddress(id);
			me->Download(renderer);
		}

		return true;
	}

    bool LayerRendererPaintPretessellated::UnloadInGPU(ImmCore::piRenderer * renderer, ImmCore::piSoundEngine * sound, ImmCore::piLog * log, Layer * la, unsigned int drawingID)
    {
        LayerPaint* lp = (LayerPaint*)la->GetImplementation();        
        const Drawing *dr = lp->GetDrawing(drawingID);
        const int id = dr->GetGpuId();
        if (id == -1) return false;
        iSLayerDrawInfoPretessellated* me = (iSLayerDrawInfoPretessellated*)mLayerInfo.GetAddress(id);
        me->Download(renderer);                
        return true;
    }

	bool LayerRendererPaintPretessellated::LoadInCPU( piLog* log, Layer* la)
	{
		LayerPaint* lp = (LayerPaint*)la->GetImplementation();

		const int numDrawings = lp->GetNumDrawings();

		int s = sizeof(iSLayerDrawInfoPretessellated);
		for (int j = 0; j < numDrawings; j++)
		{
			bool isNew = false;
			uint64_t id;
			iSLayerDrawInfoPretessellated* me = (iSLayerDrawInfoPretessellated*)mLayerInfo.Alloc(&isNew, &id, true);
			if (!me)
			{
				log->Printf(LT_ERROR, L"Couldn't alloc new DrawInfo");
				return false;
			}
            new (me) iSLayerDrawInfoPretessellated();

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

	bool LayerRendererPaintPretessellated::LoadInGPU(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la)
	{
		return true;		
	}


	void LayerRendererPaintPretessellated::GlobalWork(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la, float masterVolume)
	{

	}

	void LayerRendererPaintPretessellated::PrepareForDisplay(StereoMode stereoMode)
	{
		mStereoMode = stereoMode;
		mVisibleLayerInfos.SetLength(0);
	}

	void LayerRendererPaintPretessellated::DisplayPreRender(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la, const frustum3& frus, const trans3d & layerToViewer, float laOpacity)
	{
		LayerPaint* lp = (LayerPaint*)la->GetImplementation();
        if (!la->GetLoaded()) return;

		const Drawing *dr = lp->GetCurrentDrawing();

        if (!dr->GetLoaded())
            return;

		const bound3 bbox = dr->GetBBox();//lp->GetBBox(drawing);

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

		//---------
        uint32_t id = dr->GetGpuId();
        if (id == -1)
        {
            LoadInCPU(log, la);
        }
        id = dr->GetGpuId();
        piAssert(id != -1);

		iSLayerDrawInfoPretessellated* me = (iSLayerDrawInfoPretessellated*)mLayerInfo.GetAddress(id);

		mDrawCallInfo.numDrawCallsCulled = 0;
		mDrawCallInfo.numTrianglesCulled = 0;


		// chunk frustum culling
		bool anyVisible = false;
		for (uint32_t chunkType = 0; chunkType < kNumChunkTypes; chunkType++)
		{
            me->mBuffers[chunkType].mChunks.SetLength(0);
            if (me->mGeometry->mBuffers[chunkType].mVertices.GetLength()==0) continue;

			const uint64_t numChunks = me->mGeometry->mBuffers[chunkType].mChunks.GetLength();
			for (uint64_t i = 0; i < numChunks; i++)
			{
                const DrawingPretessellated::Geometry::Chunk *srcChunk = me->mGeometry->mBuffers[chunkType].mChunks.GetAddress(i);
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
		}
	}

	void LayerRendererPaintPretessellated::DisplayRender(piRenderer* renderer, piLog* log, piBuffer layerStateShaderConstans, int capDelta)
	{
		const uint64_t num = mVisibleLayerInfos.GetLength();
		if (num < 1) return;


		int lastShaderID = -1;
		int lastStateID = -1;

		const int stereoModeInt = static_cast<int>(mStereoMode);
		// Android uses the GL_OVR_multiview extension to render all layers of a 2D texture array.
		const int numInstances = (mStereoMode == StereoMode::Preferred && renderer->GetAPI() != piRenderer::API::GLES) ? 2 : 1;

		renderer->AttachShaderConstants(mChunkData, 9);

		mDrawCallInfo.numDrawCalls = 0;
		mDrawCallInfo.numTriangles = 0;

		renderer->AttachTextures(1, &mBlueNoise, 7);

		// for each visible layer
		for (uint64_t j = 0; j < num; j++)
		{
			const uint32_t id = mVisibleLayerInfos.GetUInt32(j);
			iSLayerDrawInfoPretessellated* me = (iSLayerDrawInfoPretessellated*)mLayerInfo.GetAddress(id);

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

            // set shader
            int shaderID = 0;
            shaderID += stereoModeInt;
#if !defined(ANDROID)
            shaderID += (me->mDrawin == true) ? 3 : 0;
#endif
            shaderID += (me->mWiggle == true) ? 6 : 0;

#if defined(ANDROID)
			shaderID += 3 * 2 * 0/*(mDebugRenderMode == Disabled ? 0 : mDebugRenderMode*/; // 4 x 6 shaders for 4 debug render modes
#endif
            if (shaderID != lastShaderID) { lastShaderID = shaderID; renderer->AttachShader(mShader[shaderID]); }

			// for each chunk in that layer
			for (uint32_t chunkType = 0; chunkType < kNumChunkTypes; chunkType++)
			{
                const iSLayerDrawInfoPretessellated::BufferData *info = me->mBuffers + chunkType;
                const uint64_t numChunks = info->mChunks.GetLength();
                if (numChunks == 0) continue;
                
                const bool doubleSided = (chunkType == static_cast<int>(Element::BrushSectionType::Segment));
				const int stateID = (doubleSided ? 0 : 1);
                // set state
                if (stateID != lastStateID) { lastStateID = stateID; renderer->SetRasterState(mRasterState[stateID]); }

                // attach vertex and index data
                if (renderer->GetAPI() == piRenderer::API::GL || renderer->GetAPI() == piRenderer::API::GLES)
                    renderer->AttachVertexArray(info->mVertexArray[0]);
                else
                    renderer->AttachVertexArray2(info->mVertexArray[stereoModeInt]);

                mDrawCallInfo.numDrawCalls += static_cast<int>(numChunks);

                // TODO: this could be done with a single DrawPrimitiveIndirect, see LaterRendererPaintStaticIndirect.cpp
				for (uint64_t i = 0; i < numChunks; i++)
				{
					const DrawingPretessellated::Geometry::Chunk *chunk = info->mChunks.Get(i);
					renderer->DrawPrimitiveIndexed(piRenderer::PrimitiveType::TriangleStrip, chunk->mNumIndices, numInstances, chunk->mVertexOffset, 0, chunk->mIndexOffset);
                    mDrawCallInfo.numIndices += chunk->mNumIndices;
                    mDrawCallInfo.numTriangles += chunk->mNumPolygons * 2;
				}
			}
		}
		renderer->DettachTextures();
		renderer->DettachShader();
		renderer->SetState(piSTATE_FRONT_FACE, true);
	}

}
