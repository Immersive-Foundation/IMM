//#define RENDER_BUDGET


#ifdef RENDER_BUDGET
#include <algorithm>
#include <src/piSdf.h>

#endif

#include "libImmCore/src/libBasics/piStr.h"
#include "libImmCore/src/libMesh/piMesh.h"
#include "libImmCore/src/libMesh/piRenderMesh.h"


#include "layerRendererModel.h"

using namespace ImmCore;
using namespace ImmImporter;

namespace ImmPlayer
{

	typedef struct
	{
		bool		 mFrontFaces;
		LayersState  mLayerState;
        piRenderMesh mRenderMesh;
        piMesh      *mMesh;
		bool         mUploaded;
        #ifdef RENDER_BUDGET
        float        mDistance;
        #endif
	}iLayerDrawInfo;

    LayerRendererModel::LayerRendererModel() : LayerRenderer() {}
    LayerRendererModel::~LayerRendererModel() {}

#if defined(WINDOWS)
#include "tmp/shader_model_vs_hlsl.inc"
#include "tmp/shader_model_fs_hlsl.inc"
#include "shader_model_vs.glsl"
#include "shader_model_fs.glsl"
#elif defined(ANDROID)
#include "tmp/shader_model_GLES_glsl.h"
#endif

	bool LayerRendererModel::Init(piRenderer* renderer, piLog* log, Drawing::ColorSpace colorSpace, bool frontIsCCW)
	{
		if (!mLayerInfo.Init(128, sizeof(iLayerDrawInfo))) // one per layer
			return false;

		if (!mVisibleLayerInfos.Init(128, sizeof(uint32_t)))
			return false;


		// SHADERS ---------------------------------------------------------
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

			if (renderer->GetAPI() == piRenderer::API::GL || renderer->GetAPI() == piRenderer::API::GLES)
			{
				const piShaderOptions opts = { 2,{
					{"COLOR_SPACE", static_cast<int>(colorSpace) },
					{"STEREOMODE", i }, } };

				mShaders[i] = renderer->CreateShader(&opts, shader_model_vs, nullptr, nullptr, nullptr, shader_model_fs, error);
				if (!mShaders[i])
				{
					log->Printf(LT_ERROR, L"Could not initalize Model layer shader\n%s", pistr2ws(error));
					return false;
				}
			}
			else
			{
#ifndef ANDROID
				const int poff = 3 * (static_cast<int>(colorSpace));
                const int vid = i + poff;
                const int fid = i;

				mShaders[i] = renderer->CreateShaderBinary(nullptr, shader_model_vs_code[vid], shader_model_vs_size[vid], nullptr, 0, nullptr, 0, nullptr, 0, shader_model_fs_code[fid], shader_model_fs_size[fid], error);
				if (!mShaders[i])
				{
					log->Printf(LT_ERROR, L"Could not initalize model layer shader\n%s", pistr2ws(error));
					return false;
				}
#endif
			}

		} // stereo mode loop



		mRasterState = renderer->CreateRasterState(false, true, piRenderer::CullMode::NONE, true, false);
		if (!mRasterState) return false;


		return true;
	}

	void LayerRendererModel::Deinit(piRenderer* renderer, piLog* log)
	{
		mVisibleLayerInfos.End();
        mLayerInfo.End();

		renderer->DestroyRasterState(mRasterState);

		if (renderer->GetAPI() == piRenderer::API::DX) return;
		for (int i = 0; i < 3; i++)
		{
			renderer->DestroyShader(mShaders[i]);
		}
	}

	bool LayerRendererModel::LoadInCPU(piLog* log, Layer* la)
	{
		LayerModel* lp = (LayerModel*)la->GetImplementation();

		bool isNew = false;
		uint64_t id = -1;
		iLayerDrawInfo* me = (iLayerDrawInfo*)mLayerInfo.Alloc(&isNew, &id, true);
		if (!me)
			return false;

        me->mMesh = lp->GetMesh();
        me->mUploaded = false;

		lp->SetGpuId(static_cast<int>(id));

		return true;
	}
	void LayerRendererModel::UnloadInCPU(piLog* log, Layer* la)
	{
		const LayerModel* lp = (LayerModel*)la->GetImplementation();
		const uint64_t id = lp->GetGpuId();
		mLayerInfo.Free(id);
	}


	bool LayerRendererModel::UnloadInGPU(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la)
	{
		const LayerModel* lp = (LayerModel*)la->GetImplementation();
		const uint64_t id = lp->GetGpuId();
		iLayerDrawInfo* me = (iLayerDrawInfo*)mLayerInfo.GetAddress(id);
		if (!me->mUploaded)
			return true;

        me->mRenderMesh.End(renderer);
		return true;
	}

	bool LayerRendererModel::LoadInGPU(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la)
	{
        LayerModel* lp = (LayerModel*)la->GetImplementation();
		const uint64_t id = lp->GetGpuId();
		iLayerDrawInfo* me = (iLayerDrawInfo*)mLayerInfo.GetAddress(id);

		return true;
	}

	static bool iUpload(piRenderer* renderer, iLayerDrawInfo* me, piLog* log)
	{
        bool staticStream = false;
        if (!me->mRenderMesh.InitFromMesh(renderer, me->mMesh, piRenderer::PrimitiveType::Triangle, &staticStream))
            return false;

		me->mUploaded = true;

		return true;
	}

	void LayerRendererModel::GlobalWork(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la, float masterVolume)
	{
	}

	void LayerRendererModel::PrepareForDisplay(StereoMode stereoMode)
	{
		mVisibleLayerInfos.SetLength(0);
		mStereoMode = stereoMode;
	}

	void LayerRendererModel::DisplayPreRender(piRenderer* renderer, piSoundEngine* sound, piLog* log, Layer* la, const frustum3& frus, const trans3d & layerToViewer, float laOpacity)
	{
        LayerModel* lp = (LayerModel*)la->GetImplementation();
		const uint32_t id = lp->GetGpuId();
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

	void LayerRendererModel::DisplayRender(piRenderer* renderer, piLog* log, piBuffer layerStateShaderConstans, int capDelta)
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

		for (uint64_t j = 0; j < num; j++)
		{
			const uint32_t id = mVisibleLayerInfos.GetUInt32(j);
			iLayerDrawInfo* me = (iLayerDrawInfo*)mLayerInfo.GetAddress(id);

			renderer->UpdateBuffer(layerStateShaderConstans, &me->mLayerState, 0, sizeof(LayersState));

			//const float data[4] = { 0.0f, 1.0f, 0.0f, 0.0f };

			const int idStereo = static_cast<int>(mStereoMode);

			renderer->SetState(piSTATE_FRONT_FACE, me->mFrontFaces);

			if (!me->mUploaded)
			{
				if (!iUpload(renderer, me, log))
					continue;
			}


			renderer->AttachShader(mShaders[idStereo]);
			me->mRenderMesh.Render( renderer, 0, 0, numInstances);
			renderer->DettachShader();
		}
		renderer->SetState(piSTATE_CULL_FACE, true);
		renderer->SetState(piSTATE_FRONT_FACE, true);
	}

}
