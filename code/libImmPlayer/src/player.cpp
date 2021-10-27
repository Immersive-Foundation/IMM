#define CUSTOM_ALPHA_TO_COVERAGE 1
#define UNITY
#include <malloc.h>
#include <thread>
#include <chrono>


#include "libImmCore/src/libBasics/piDebug.h"
#include "libImmCore/src/libBasics/piFileName.h"
#include "libImmCore/src/libBasics/piStr.h"


#include "player.h"
#include "libImmImporter/src/document/layer.h"
#include "libImmImporter/src/document/layerEffect.h"
#include "libImmImporter/src/document/layerInstance.h"
#include "libImmImporter/src/document/layerPaint.h"
#include "libImmImporter/src/document/sequence.h"
#include "libImmImporter/src/fromImmersive/fromImmersive.h"
#include "layerRenderers/layerRendererPaint/pretessellated/layerRendererPaintPretessellated.h"
#include "layerRenderers/layerRendererPaint/static/layerRendererPaintStatic.h"

#include "blue_noise.h"
#include "document.h"

using namespace ImmCore;
using namespace ImmImporter;

namespace ImmPlayer
{
    static const wchar_t *kRenderingTechniques[] = { L"static", L"pretessellated" };

    Player::Player() {}

    Player::~Player() {}

    bool Player::Init(piRenderer* renderer, piSoundEngine* sound, piLog* log, piTimer *timer, const Configuration * configuration)
    {
        log->Printf(LT_MESSAGE, L"Player Init...");

#if !defined(ANDROID)
        if (configuration->multisamplingLevel != 8)
        {
            log->Printf(LT_ERROR, L"We only suport 8xAA");
            return false;
        }
#endif

        #ifdef RENDER_BUDGET
        mMicrosecondsBudget = 0;
        mMicrosecondsLastFrame = 0;
        mDeltaCapTrigger = 0;
        #endif
        mDeltaCap = 0;

        mRenderer = renderer;
        mSoundEngine = sound;
        mLog = log;
        mTimer = timer;
        mFrame = 0;
        mEnabled = true;
		mAnyDocToRender = false;
        mDetphBufferMode = configuration->depthBuffer;
        mColorSpace = configuration->colorSpace;
        mClipDepthMode = configuration->clipDepth;
        mProjectionMatricesMode = configuration->projectionMatrix;
        mPaintRenderingTechnique = configuration->paintRenderingTechnique;
        mSeed = 13;

        if (!mDocuments.Init(32, sizeof(Document)))
            return false;
        if (!mCommandList.Init(32, false))
            return false;
        if (!mSynced.Init(32, false))
            return false;
        for (int i = 0; i < mSynced.GetLength(); i++)
        {
            mSynced[i] = false;
        }

        //----------------------------------------------------------------------------------------------------------------------------------------

        mFrameStateShaderConstans = renderer->CreateBuffer(nullptr, sizeof(FrameState), piRenderer::BufferType::Dynamic, piRenderer::BufferUse::Constant);
        if (!mFrameStateShaderConstans) return false;

        mDisplayStateShaderConstans = renderer->CreateBuffer(nullptr, sizeof(DisplayRenderState), piRenderer::BufferType::Dynamic, piRenderer::BufferUse::Constant);
        if (!mDisplayStateShaderConstans) return false;

        mLayerStateShaderConstans = renderer->CreateBuffer(nullptr, sizeof(LayersState), piRenderer::BufferType::Dynamic, piRenderer::BufferUse::Constant);
        if (!mLayerStateShaderConstans) return false;

        mPassStateShaderConstans = renderer->CreateBuffer(nullptr, sizeof(PassState), piRenderer::BufferType::Dynamic, piRenderer::BufferUse::Constant);
        if (!mPassStateShaderConstans) return false;

        mGlobalResourcesConstans = renderer->CreateBuffer(0, sizeof(GlobalResourcesState), piRenderer::BufferType::Dynamic, piRenderer::BufferUse::Constant);
        if (!mGlobalResourcesConstans) return false;


        // Blue noise
        if (renderer->GetAPI() == piRenderer::API::GL)
        {
            const piRenderer::TextureInfo infob = { piRenderer::TextureType::T2D_ARRAY, piRenderer::Format::C1_8_UNORM, 64, 64, 64, 1 };
            mBlueNoise = renderer->CreateTexture(0, &infob, false, piRenderer::TextureFilter::NONE, piRenderer::TextureWrap::REPEAT, 1.0f, (void*)GetBlueNoise_64x64x64());
            if (mBlueNoise == nullptr)
                return false;

            GlobalResourcesState globalResources;
            globalResources.mBlueNoiseTexture = mRenderer->GetTextureHandle(mBlueNoise);
            mRenderer->MakeResident(mBlueNoise);

            mRenderer->UpdateBuffer(mGlobalResourcesConstans, &globalResources, 0, sizeof(GlobalResourcesState));
        }

        //----------------------------------------------------------------------------------------------------------------------------------------

        // render states
        if (renderer->GetAPI() == piRenderer::API::DX)
        {
            mRasterState = renderer->CreateRasterState(false,true, piRenderer::CullMode::NONE, true, false); // note multisample is set to false
            if (!mRasterState) return false;

            mDepthState = renderer->CreateDepthState(true, (mDetphBufferMode==DepthBuffer::Linear01)?true:false); // set the second to true for non-unity DX. I know.
            if (!mDepthState) return false;

#if CUSTOM_ALPHA_TO_COVERAGE==1
            mBlendState = renderer->CreateBlendState(false, false);
#else
            mBlendState = renderer->CreateBlendState(true, false);
#endif
            if (!mBlendState) return false;
        }
        //----------------------------------------------------------------------------------------------------------------------------------------

        switch (configuration->paintRenderingTechnique)
        {
            case Drawing::Pretessellated:
                mLayerPaintRender = new LayerRendererPaintPretessellated();
                break;
            case Drawing::Static:
                mLayerPaintRender = new LayerRendererPaintStatic();
                break;
            default:
                return false;
        }

        if (!mLayerPaintRender->Init(renderer, log, mColorSpace, configuration->frontIsCCW)) { log->Printf(LT_ERROR, L"Could not Init LayerPaintRender");   return false; };
        if (!mLayerRenderPicture.Init(renderer, log, mColorSpace, configuration->frontIsCCW)) { log->Printf(LT_ERROR, L"Could not Init LayerRenderPicture"); return false; };
        if (!mLayerRenderSound.Init(renderer, log, mColorSpace, configuration->frontIsCCW)) { log->Printf(LT_ERROR, L"Could not Init LayerRenderSound"); return false; };
        if (!mLayerRenderModel.Init(renderer, log, mColorSpace, configuration->frontIsCCW)) { log->Printf(LT_ERROR, L"Could not Init LayerRenderModel"); return false; };

        //----------------------------------------------------------------------------------------------------------------------------------------

        log->Printf(LT_MESSAGE, L"Player Init OK!");

        piAssert( (mPaintRenderingTechnique >=0) && (mPaintRenderingTechnique <= (sizeof(kRenderingTechniques)/sizeof(wchar_t*))));

        if( !mCurrentPerfInfo.paintRenderingStrategy.InitCopyW(kRenderingTechniques[mPaintRenderingTechnique]) )
            return false;

        return true;
    }

    void Player::Deinit(void)
    {
        const uint64_t num = mDocuments.GetMaxLength();
        for(uint64_t i=0; i<num; i++)
        {
            if (!mDocuments.IsUsed(i)) continue;
            Document *doc = (Document*)mDocuments.GetAddress(i);

            // Force unload of document CPU resources on deinit.
            // Note that GPU resources will be freed when the GL context is destroyed so we may not
            // need to explicitly free GPU resources if we know the context is going away.
            // Freeing of GPU resources needs to happen on the render thread which may or may not be
            // the same thread as the main thread.
            if(doc->GetLoadingState() == Document::LoadingState::Loaded)
                doc->UnloadCPU(mLayerPaintRender, &mLayerRenderPicture, mLog);
            doc->End();

        }
        mDocuments.End();

        mLog->Printf(LT_MESSAGE, L"Player Deinit...");

        mCurrentPerfInfo.paintRenderingStrategy.End();

        mLayerPaintRender->Deinit(mRenderer, mLog);
        delete mLayerPaintRender;
        mLayerRenderPicture.Deinit(mRenderer, mLog);
        mLayerRenderSound.Deinit(mRenderer, mLog);
        mLayerRenderModel.Deinit(mRenderer, mLog);

        if (mRenderer->GetAPI() == piRenderer::API::GL)
        {
            mRenderer->DestroyTexture(mBlueNoise);
        }

        mRenderer->DestroyBuffer(mGlobalResourcesConstans);
        mRenderer->DestroyBuffer(mLayerStateShaderConstans);
        mRenderer->DestroyBuffer(mFrameStateShaderConstans);
        mRenderer->DestroyBuffer(mDisplayStateShaderConstans);
        mRenderer->DestroyBuffer(mPassStateShaderConstans);

        mCommandList.End();
        mSynced.End();

        mLog->Printf(LT_MESSAGE, L"Player Deinit OK!");
    }

    // TODO: move it somewhere?
    enum class CapsFlags : uint32_t
    {
        Grabbable = 1 << 0,
        Soundable = 1 << 1,
    };

    #define IS_FLAG_SET(v, f) ((bool)((((v) & ((uint32_t)(f))) == ((uint32_t)(f))) ? 1 : 0))
    #define SET_FLAG(v, f, b) ((bool)(b) ? ((v) = ((v) | (f))) : ((v) = (v) & (~f)))
    uint32_t Player::GetDocumentInfoEx(int id) const
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);

        // TODO: this should be read from the metadata of the file instead of the sequence
        const Sequence *sq = doc->GetSequence();
        const Sequence::Type type = sq->GetType();
        const uint16_t caps = sq->GetCaps();

        uint32_t info = 0;
        SET_FLAG(info, (uint32_t)DocumentInfoFlags::MOVABLE, true);
        SET_FLAG(info, (uint32_t)DocumentInfoFlags::DISPLAYABLE, true);
        SET_FLAG(info, (uint32_t)DocumentInfoFlags::PLAYABLE, ((Sequence::Type::Animated == type) || (Sequence::Type::Comic == type)));
        SET_FLAG(info, (uint32_t)DocumentInfoFlags::NEXTABLE, (Sequence::Type::Comic == type));
        SET_FLAG(info, (uint32_t)DocumentInfoFlags::PREVABLE, (Sequence::Type::Comic == type));
        SET_FLAG(info, (uint32_t)DocumentInfoFlags::TIMEABLE, (Sequence::Type::Comic == type));
        SET_FLAG(info, (uint32_t)DocumentInfoFlags::SOUNDABLE, IS_FLAG_SET(caps, CapsFlags::Soundable));
        SET_FLAG(info, (uint32_t)DocumentInfoFlags::BOUNDABLE, true);
        SET_FLAG(info, (uint32_t)DocumentInfoFlags::GRABBABLE, IS_FLAG_SET(caps, CapsFlags::Grabbable));
        SET_FLAG(info, (uint32_t)DocumentInfoFlags::VIEWABLE, true);
        return info;
    }
    #undef SET_FLAG
    #undef IS_FLAG_SET

    void Player::GetDocumentState(DocumentState & state, int id) const
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        if (doc == nullptr) {
            mLog->Printf(LT_ERROR, L"Null document at %d.", id);
            state.mLoadingState = LoadingState::Failed;
            return;
        }

        Document::ErrorState   est = doc->GetErrorState();
        Document::LoadingState lst = doc->GetLoadingState();

        if (est == Document::ErrorState::NoError)
        {
#ifdef UNITY
            // In Unity, GlobalWork() is running in LateUpdate(), so we need to use mSynced as a flag to force Unity to wait for another frame
            // before can load any new document. This would not change the behavior for standalone player.
            if (lst == Document::LoadingState::LoadingPending || (lst == Document::LoadingState::UnloadingCompleted && mSynced[id]))
            {
                mLog->Printf(LT_DEBUG, L"document %d is unloaded and synced.", id);
                state.mLoadingState = LoadingState::Unloaded;
            }
#else
            if (lst == Document::LoadingState::LoadingPending || (lst == Document::LoadingState::UnloadingCompleted))
            {
                state.mLoadingState = LoadingState::Unloaded;
            }
#endif // UNITY
            else if (lst == Document::LoadingState::LoadingCPU || lst == Document::LoadingState::LoadingSPU || lst == Document::LoadingState::LoadingGPU || lst == Document::LoadingState::LoadingComplete)
            {
                state.mLoadingState = LoadingState::Loading;
            }
            else if (lst == Document::LoadingState::Loaded)
            {
                state.mLoadingState = LoadingState::Loaded;
            }
            else// if (lst == Document::LoadingState::UnloadingCPU || lst == Document::LoadingState::UnloadingSPU || lst == Document::LoadingState::UnloadingGPU)
            {
                state.mLoadingState = LoadingState::Unloading;
            }
        }
        else
        {
            state.mLoadingState = LoadingState::Failed;
        }

        state.mPlaybackState = static_cast<Player::PlaybackState>(doc->GetPlaybackState());
    }

    void Player::GetPlayerInfo(PlayerInfo & info) const
    {
        vec3 col = vec3(0.0f, 0.0f, 0.0f);

        const uint64_t num = mDocuments.GetMaxLength();
        for (uint64_t i = 0; i < num; i++)
        {
            if (!mDocuments.IsUsed(i)) continue;
            Document *doc = (Document *)mDocuments.GetAddress(i);
            const Document::LoadingState st = doc->GetLoadingState();
            if (st == Document::LoadingState::Loaded )
            {
                col = doc->GetSequence()->GetBackgroundColor();
            }
        }

        if (mColorSpace == Drawing::ColorSpace::Gamma)
        {
            col = pow(col, 1.0f / 2.2f);
        }

        info.mBackgrundColor.mRed = col.x;
        info.mBackgrundColor.mGreen = col.y;
        info.mBackgrundColor.mBlue = col.z;
    }

    void Player::CancelLoading(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        doc->CancelLoading();
    }

	void Player::GetChapterInfo(size_t& numChapters, piTArray<piTick>& chapterLengths, bool& hasPlays, int id)
	{
		Document *doc = (Document *)mDocuments.GetAddress(id);
		const Sequence *sq = doc->GetSequence();
		numChapters = doc->GetChapterCount();
		hasPlays = doc->GetHasPlays();

		Layer* root = sq->GetRoot();
		const unsigned int numActionKeys = root->GetNumAnimKeys(Layer::AnimProperty::Action);
		piTick prev = piTick(0);
		for (unsigned int i = 0; i < numActionKeys; i++)
		{
			Layer::AnimKey* key = (Layer::AnimKey*) root->GetAnimKey(Layer::AnimProperty::Action, i);
			Layer::AnimAction action = static_cast<Layer::AnimAction>(key->mValue.mInt);

			if (hasPlays)
			{
				if (action == Layer::AnimAction::Play)
				{
					piTick* length = chapterLengths.Alloc(1, true);
					*length = key->mTime - prev;
					prev = key->mTime;
				}
			}
			else
			{
				if (action == Layer::AnimAction::Stop)
				{
					piTick* length = chapterLengths.Alloc(1, true);
					*length = key->mTime - prev;
					prev = key->mTime;
				}
			}

		}
		// End
		piTick* length = chapterLengths.Alloc(1, true);
		//if (hasPlays)
		//{
		//	// when using plays to define chapters, if the there is a stop at the end, use that as story end.
		//	Layer::AnimKey* key = (Layer::AnimKey*) root->GetAnimKey(Layer::AnimProperty::Action, numActionKeys - 1);
		//	if (static_cast<Layer::AnimAction>(key->mValue.mInt) == Layer::AnimAction::Stop)
		//	{
		//		*length = key->mTime - prev;
		//		return;
		//	}
		//}
		// default story length 1hr
		*length = piTick(45360000) - prev;
	}

    void Player::SetDocumentToWorld(int id, const trans3d & m)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);

        doc->SetDocumentToWorld(m);
    }

    int Player::GetSpawnAreaCount(int docId)
    {
        Document *doc = (Document *)mDocuments.GetAddress(docId);
        return doc->GetSpawnAreaCount();
    }

    int Player::GetSpawnArea(int docId)
    {
        Document *doc = (Document *)mDocuments.GetAddress(docId);
        return doc->GetSpawnArea();
    }

    int Player::GetInitialSpawnArea(int docId)
    {
        Document *doc = (Document *)mDocuments.GetAddress(docId);
        return doc->GetInitialSpawnArea();
    }

    void Player::SetSpawnArea(int docId, int spawnAreaId)
    {
        Document *doc = (Document *)mDocuments.GetAddress(docId);
        doc->SetSpawnArea(spawnAreaId);
    }

    bool Player::GetSpawnAreaNeedsUpdate(int docId)
    {
        Document *doc = (Document *)mDocuments.GetAddress(docId);
        return doc->GetSpawnAreaNeedsUpdate();
    }

    void Player::SetSpawnAreaNeedsUpdate(int docId, bool state)
    {
        Document *doc = (Document *)mDocuments.GetAddress(docId);
        doc->SetSpawnAreaNeedsUpdate(state);
    }

    const piImage* Player::GetSpawnAreaScreenshot(int docId, int spawnAreaId)
    {
        Document *doc = (Document *)mDocuments.GetAddress(docId);
        return doc->GetSpawnAreaScreenshot(spawnAreaId);
    }

    bool Player::GetSpawnAreaInfo(Document::SpawnAreaInfo & info, int docId, int spawnAreaId)
    {
        Document *doc = (Document *)mDocuments.GetAddress(docId);
        if (!doc || spawnAreaId >= doc->GetSpawnAreaCount())
        {
            mLog->Printf(LT_ERROR, L"Invalid document id %d or spawnarea id %d.", docId, spawnAreaId);
            return false;
        }
        doc->GetSpawnAreaInfo(info, spawnAreaId);
        return true;
    }

    static const Document::Command cmdHide = { Document::Command::Type::Hide };
    static const Document::Command cmdShow = { Document::Command::Type::Show };

    void Player::GlobalWork( bool enabled, uint32_t microsecondsBudget)
    {
        const uint64_t num = mDocuments.GetMaxLength();

        #ifdef RENDER_BUDGET
        mMicrosecondsBudget = microsecondsBudget;

        if (mMicrosecondsLastFrame < ((microsecondsBudget*95)/100) ) // we underusing our budget, lets start adding more geometry
        {
            mDeltaCap = 0;
            mDeltaCapTrigger++;
            if (mDeltaCapTrigger > 10) // but only with some hysteresis to prevent unstabilities
            {
                mDeltaCap = 1;
                mDeltaCapTrigger = 0;
            }
        }
        else if (mMicrosecondsLastFrame > microsecondsBudget) // we are over budget. Start removing geometry immediatelly
        {
            mDeltaCap = -1;
            mDeltaCapTrigger = 0;
        }
        //mLog->Printf(LT_MESSAGE, L"%d %.1f", mDeltaCap, float(mMicrosecondsLastFrame)/1000.0f);
        #endif

        //-----------------------------
        // set global info
        //-----------------------------
        mTime = int64_t(mTimer->GetTimeTicks());
        mFrameState.mTime = static_cast<float>(piTick::ToSeconds(mTime));
        mFrameState.mFrameID = mFrame++;

        // We lock here to prevent updating the scenegraphs while the renderer is going.
        // Ideally we can do this with double buffering. But we are in a hurry right now
        bool anyDocReady = false;
        mMutex.lock();
        {

            // handle global player state
            const bool needDisable = (mEnabled && !enabled);
            const bool needEnable = (!mEnabled && enabled);
            mEnabled = enabled;


            // send commands to documents
            {

                for (uint64_t currDocId = 0; currDocId < num; currDocId++)
                {
                    if (!mDocuments.IsUsed(currDocId)) continue;

                    Document *doc = (Document *)mDocuments.GetAddress(currDocId);
                    int cmdId = doc->GetCommandId();

                    if (doc->GetLoadingState() == Document::LoadingState::UnloadingCompleted)
                    {
                        // free fully unloaded documents
                        mDocuments.Free(currDocId);
                        mCommandList.RemoveAndShift(cmdId);
                        for (int i = cmdId; i < mCommandList.GetLength(); i++)
                        {
                            int d = mCommandList[i].mTarget;

                            if (d != -1 && mDocuments.IsUsed(d))
                            {
                                Document *doc = (Document *)mDocuments.GetAddress(d);
                                doc->SetCommandId(i);
                            }
                        }
                        mSynced[currDocId] = true;
                        continue;
                    }


                    const Document::Command *cmd = nullptr;
                    if ((mCommandList[cmdId].mCommand.mType != Document::Command::Type::None) &&
                        (mCommandList[cmdId].mTarget == currDocId))
                    {
                        cmd = &mCommandList[cmdId].mCommand;
                    }

                    if (needDisable)
                    {
                        cmd = &cmdHide;
                    }
                    if (needEnable)
                    {
                        cmd = &cmdShow;
                    }

                    // update loading process OR animation scenegraph
                    bool docReady = doc->UpdateStateCPU(&mLayerRenderSound, mLayerPaintRender, &mLayerRenderPicture, &mLayerRenderModel, mColorSpace, mPaintRenderingTechnique, mSoundEngine, mLog, mTime, cmd);
                    if (docReady)
                    {
                        iGlobalWorkLayer(doc->GetSequence()->GetRoot(), doc->GetVolume());
                    }
                    anyDocReady |= docReady;

                    // reset the command
                    mCommandList[cmdId].mCommand.mType = Document::Command::Type::None;
                    //mCommandList[cmdId].mTarget = -1;
                }
            }
        }
        mMutex.unlock();

        if (anyDocReady)
        {
            if (mSoundEngine != nullptr)
            {
                trans3d tmp = mViewerInfo.mHeadToWorld;
                tmp.mScale = 1.0;
                mSoundEngine->SetListener(tmp);

            }
        }
    }

    // this is called once per camera
    void Player::GlobalRender(const trans3d & vr_to_head, const trans3d & world_to_head, const mat4x4 & projection, const StereoMode & stereoMode)
    {
        if (!mEnabled) return;

        //mLog->Printf(LT_MESSAGE, L"GlobalRender(%d)", mFrameState.mFrameID);

        const uint64_t num = mDocuments.GetMaxLength();

        // ---- setup viewer info -----------------------------------------------
        mViewerInfo.mVRToHead = vr_to_head;
        mViewerInfo.mWorldToHead = world_to_head;// *mViewerInfo.mPlayerCamera;
        mViewerInfo.mHeadToWorld = invert(mViewerInfo.mWorldToHead);
        mViewerInfo.mCamPos = (mViewerInfo.mHeadToWorld * vec4d(0.0f, 0.0f, 0.0f, 1.0f)).xyz();
        mViewerInfo.mCamDir = normalize((mViewerInfo.mHeadToWorld * vec4d(0.0f, 0.0f, -1.0f, 0.0f)).xyz());
        mViewerInfo.mProjection = projection;

        //--- upload global info to the GPU --------------------------------------

        mRenderer->UpdateBuffer(mFrameStateShaderConstans, &mFrameState, 0, sizeof(FrameState));

        //------------------------------------------------------
        // view independant rendering here
        //------------------------------------------------------

        mRenderer->AttachShaderConstants(mFrameStateShaderConstans, 0);
        mRenderer->AttachShaderConstants(mLayerStateShaderConstans, 3);
        mRenderer->AttachShaderConstants(mDisplayStateShaderConstans, 4);
        mRenderer->AttachShaderConstants(mPassStateShaderConstans, 5);
        mRenderer->AttachShaderConstants(mGlobalResourcesConstans, 7);

        mLayerPaintRender->PrepareForDisplay(stereoMode);
        mLayerRenderPicture.PrepareForDisplay(stereoMode);
        mLayerRenderSound.PrepareForDisplay(stereoMode);
        mLayerRenderModel.PrepareForDisplay(stereoMode);

        mCurrentPerfInfo.numDrawCalls = 0;
        mCurrentPerfInfo.numDrawCallsCulled = 0;
        mCurrentPerfInfo.numTriangles = 0;
        mCurrentPerfInfo.numTrianglesCulled = 0;

        // this block below, which is per camera, takes the data from the scenegraph (which is updated by the animation manager) and
        // uses it to compile the rendering data that will be needed later. This includes doing the frustum culling but also preparing
        // the actual Layer state and matrices in such a way that the rendering of the frame from this point on will not depend on the scene
        // graph anymore. That allows us to run the scenegraph in parallel to the rendering without further mutex protection
        mMutex.lock();
        {
            bool anyDocReady = false;
            for (uint64_t i = 0; i < num; i++)
            {
                if (!mDocuments.IsUsed(i)) continue;
                Document *doc = (Document *)mDocuments.GetAddress(i);

                // update loading process ideally this happens only for the first camera. We need to have a frameID counter for that to detect changes in frameID
                const bool needRender = doc->UpdateStateGPU(mLayerPaintRender, &mLayerRenderPicture, &mLayerRenderModel, mRenderer, mLog, mColorSpace);
                anyDocReady |= needRender;

                if (needRender)
                {
                    iDisplayPreRenderLayer(doc->GetSequence()->GetRoot(), doc->GetDocumentToWorld(), 1.0f, mViewerInfo.mWorldToHead);

                    iUnloadNotInTimeline(doc->GetSequence()->GetRoot(), mTime);
                }
            }

            if (!mAnyDocToRender && anyDocReady)
            {
                // Records from the initial load request to when the first document is marked as ready to render.
                mCurrentPerfInfo.cpuLoadTimeMS = GetLoadTimeInMs();
            }
            mAnyDocToRender = anyDocReady;
        }
        mMutex.unlock();

        //log->Printf(LT_MESSAGE, L"Global Done!");
    }

    void Player::iGlobalWorkLayer(Layer* la, float masterVolume)
    {
        const Layer::Type lt = la->GetType();

        if (lt == Layer::Type::Group)
        {
            const int num = la->GetNumChildren();
            for (int i = 0; i < num; i++)
            {
                Layer* lc = la->GetChild(i);
                iGlobalWorkLayer(lc, masterVolume);
            }
            return;
        }

        // we need to process sounds no matter what (so we can stop them)
        // that includes groups that may contain sounds, so we can only
        // early exit after parsing groups
        if (lt != Layer::Type::Sound && !la->GetVisible()) return;

        LayerRenderer *lr = nullptr;
        if (lt == Layer::Type::Paint)   lr = mLayerPaintRender;
        else if (lt == Layer::Type::Picture) lr = &mLayerRenderPicture;
        else if (lt == Layer::Type::Sound)   lr = &mLayerRenderSound;
        else if (lt == Layer::Type::Model)   lr = &mLayerRenderModel;

        if (lr != nullptr) lr->GlobalWork(mRenderer, mSoundEngine, mLog, la, masterVolume);
    }

    static mat4x4d gl2dx(const mat4x4d & mat)
    {
        return mat4x4d(mat[0], mat[1], mat[2], mat[3],
            mat[4], -mat[5], mat[6], mat[7],
            mat[8], mat[9], -0.5*(1.0 + mat[10]), -0.5*mat[11],
            mat[12], mat[13], mat[14], mat[15]);
    }

    static mat4x4d dx2gl(const mat4x4d & mat)
    {
        return mat4x4d(mat[0], mat[1], mat[2], mat[3],
            mat[4], -mat[5], mat[6], mat[7],
            mat[8], mat[9], -(1.0 + 2.0*mat[10]), -2.0*mat[11],
            mat[12], mat[13], mat[14], mat[15]);
    }

    void Player::iDisplayPreRenderLayer(Layer* la, const trans3d & parentToWorld, float parentOpacity, const trans3d & worldToViewer)
    {
        if (!la->GetVisible())
            return;


        if (!la->GetPotentiallyVisible())
        {
            return;
        }

        const trans3d layerToWorld = parentToWorld * la->GetTransform();

        const float laOpacity = parentOpacity * la->GetOpacity();

        if (laOpacity == 0.0f)
        {
            return;
        }
        const Layer::Type lt = la->GetType();

        trans3d layerToViewer = worldToViewer * layerToWorld;

        // we need to do this here so frustum culling is correct with viewer locked images
        if (lt == Layer::Type::Picture)
        {
            LayerPicture * lp = (LayerPicture*) la->GetImplementation();
            if (lp->GetIsViewerLocked())
            {
                // layer transform is in VR space
                layerToViewer = mViewerInfo.mVRToHead * la->GetTransform();
                if (lp->GetType() != LayerPicture::Image2D)
                {
                    // 360 snap to viewer / origin
                    layerToViewer.mTranslation = vec3d(0.0);
                }
            }
        }

        // TODO: do group frustum culling here to skip whole sections of the scenegraph

        //const frustum3d dfrus = frustum3d( f2d(mViewerInfo.mProjection) * layerToViewer);

        mat4x4d tmp = f2d(mViewerInfo.mProjection);
        // DX to GL conversion. If we remove this, then enable dx2gl() in player.cpp::iDisplayPreRenderLayer::289 and also enable GL.GetGPUProjectionMatrix() in C#
        if (mProjectionMatricesMode==ClipSpaceDepth::FromZeroToOne) tmp = dx2gl(tmp);
        const frustum3d dfrus = frustum3d(tmp * toMatrix(layerToViewer));

        const frustum3 frus = d2f(dfrus); // note the conversion to float _once_ we are in viewer space

        LayerRenderer *lr = nullptr;

        if (lt == Layer::Type::Group)
        {
            const int num = la->GetNumChildren();
            for (int i = 0; i < num; i++)
            {
                Layer* lc = la->GetChild(i);
                iDisplayPreRenderLayer(lc, layerToWorld, laOpacity, worldToViewer);
            }
            return;
        }
        else if (lt == Layer::Type::Paint) { lr = mLayerPaintRender; }
        else if (lt == Layer::Type::Picture) { lr = &mLayerRenderPicture; }
        else if (lt == Layer::Type::Sound) { lr = &mLayerRenderSound; }
        else if (lt == Layer::Type::Model) { lr = &mLayerRenderModel; }

        if (lr == nullptr) return;

        lr->DisplayPreRender(mRenderer, mSoundEngine, mLog, la, frus, layerToViewer, laOpacity);

#if defined(ANDROID)
        //lr->SetDebugRenderMode(mDebugRenderMode);

        mCurrentPerfInfo.numDrawCallsCulled += lr->GetDrawCallInfo().numDrawCallsCulled;
        mCurrentPerfInfo.numTrianglesCulled += lr->GetDrawCallInfo().numTrianglesCulled;
#endif
    }

    void Player::iUnloadNotInTimeline(Layer * root, piTick now)
    {
        if (!root->GetPlaying()) return;
        piTick rootTime = now - root->GetStartTime();

        auto checkUsage = [this, &rootTime](Layer* layer, int level, int child, bool instance) -> bool
        {
            if (!layer->GetLoaded()) return true;

            bool visible = false;
            piTick offset = piTick(0);
            layer->GetLocalTimeOffsetAndVisiblity(rootTime, &offset, &visible);

            if (layer->GetType() == Layer::Type::Paint)
            {
                LayerPaint* lp = (LayerPaint*)layer->GetImplementation();
                const unsigned int numDrawings = lp->GetNumDrawings();
                const Drawing* currentDrawing = lp->GetCurrentDrawing();
                for (unsigned int i = 0; i < numDrawings; i++)
                {
                    Drawing* dr = lp->GetDrawing(i);
                    if ( ((dr != currentDrawing) || (!visible) ) && dr->GetGpuId() != -1)
                        mLayerPaintRender->UnloadInGPU(mRenderer, mSoundEngine, mLog, layer, i);
                }

            }
            else if (layer->GetType() == Layer::Type::Picture)
            {
                if (!visible)
                    mLayerRenderPicture.UnloadInGPU(mRenderer, mSoundEngine, mLog, layer);
            }
            return true;
        };

        root->Recurse(checkUsage, 0, 0, false, false, false, false, false);
    }

    mat4x4 Player::iConvertProjectionMatrix( const mat4x4 & mat)
    {
        if (mClipDepthMode == ClipSpaceDepth::FromZeroToOne && mProjectionMatricesMode == ClipSpaceDepth::FromNegativeOneToOne)
        {
            // if a DX host is using OpenGL style matrices (the case when the ImmPlayer host is in DX mode)
            static const mat4x4 kk = mat4x4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.5f, 0.5f,
                0.0f, 0.0f, 0.0f, 1.0f);
            return kk * mat;
        }
        else if (mClipDepthMode == ClipSpaceDepth::FromNegativeOneToOne && mProjectionMatricesMode == ClipSpaceDepth::FromZeroToOne)
        {
            // could happen if a OpenGL host is using DX-style projection matrices for their maths
            piAssert(false);// not implemented yet
        }
        else
        {
            // nothing to do, host's expceted projection to clip space matches the projectio matrices format
        }

        return mat;
    }

    void Player::PopulateDisplayRenderPerfInfo()
    {
#if defined(ANDROID)
        LayerRenderer::DrawCallInfo paintInfo = mLayerPaintRender->GetDrawCallInfo();
        LayerRenderer::DrawCallInfo pictureInfo = mLayerRenderPicture.GetDrawCallInfo();
        LayerRenderer::DrawCallInfo modelInfo = mLayerRenderModel.GetDrawCallInfo();

        const int totalDrawCalls = paintInfo.numDrawCalls + pictureInfo.numDrawCalls + modelInfo.numDrawCalls;
        const int totalTriangles = paintInfo.numTriangles + pictureInfo.numTriangles + modelInfo.numTriangles;

        mCurrentPerfInfo.numDrawCalls = totalDrawCalls;
        mCurrentPerfInfo.numTriangles = totalTriangles;

#if defined(RENDER_BUDGET) || defined(MEASURE_GPU_TIME)
        if (mEnablePerformanceMeasurement && mMicrosecondsLastFrame > 0)
        {
            mCurrentPerfInfo.totalGPUTimeAcrossFrames += mMicrosecondsLastFrame;
            ++mCurrentPerfInfo.numFramesMeasured;

            const int numFramesToAverage = 20;
            // Write out an average GPU time every |numFramesToAverage| frames so the value doesn't constantly fluctuate in UI.
            if (mCurrentPerfInfo.numFramesMeasured % numFramesToAverage == 0)
            {
                mCurrentPerfInfo.gpuTimeAverageMs =
                        mCurrentPerfInfo.totalGPUTimeAcrossFrames / (numFramesToAverage * 1000);
                mCurrentPerfInfo.numFramesMeasured = 0;
                mCurrentPerfInfo.totalGPUTimeAcrossFrames = 0;
            }
        }
#endif

#endif
    }

    //-----------------------------------------------------------------------------------------------------------------
    void Player::RenderMono(const ivec2 & pixelResolutionIncludingSupersampling, int eyeIndex)
    {
        if (!mEnabled) return;
        if (!mAnyDocToRender) return;

        #if defined(RENDER_BUDGET) || defined(MEASURE_GPU_TIME)
        if (mEnablePerformanceMeasurement)
            mRenderer->StartPerformanceMeasure();
        #endif

        mRenderer->AttachShaderConstants(mFrameStateShaderConstans, 0);
        mRenderer->AttachShaderConstants(mLayerStateShaderConstans, 3);
        mRenderer->AttachShaderConstants(mDisplayStateShaderConstans, 4);
        mRenderer->AttachShaderConstants(mPassStateShaderConstans, 5);
        mRenderer->AttachShaderConstants(mGlobalResourcesConstans, 7);

        //mLog->Printf(LT_MESSAGE, L"RenderMono(%d)", mFrame);

        mDisplayRenderState.mEye[0].mViewerToEye_Prj = iConvertProjectionMatrix(mViewerInfo.mProjection);
        mDisplayRenderState.mEye[1].mViewerToEye_Prj = iConvertProjectionMatrix(mViewerInfo.mProjection);
        mDisplayRenderState.mResolution = vec2(float(pixelResolutionIncludingSupersampling.x), float(pixelResolutionIncludingSupersampling.y));
        mDisplayRenderState.mEyeIndex = eyeIndex;

        mRenderer->UpdateBuffer(mDisplayStateShaderConstans, &mDisplayRenderState, 0, sizeof(DisplayRenderState));

        //mPassState.mID = 0;
        //mRenderer->UpdateBuffer(mPassStateShaderConstans, &mPassState, 0, sizeof(PassState));

        //log->Printf(LT_MESSAGE, L"DISPLAY:"); for (int i = 0; i<4; i++) log->Printf(LT_MESSAGE, L"%f %f %f %f", mDisplayRenderState.mEye[0].mMatrix_CamPrj[4 * i + 0], mDisplayRenderState.mEye[0].mMatrix_CamPrj[4 * i + 1], mDisplayRenderState.mEye[0].mMatrix_CamPrj[4 * i + 2], mDisplayRenderState.mEye[0].mMatrix_CamPrj[4 * i + 3]);

        if (mRenderer->GetAPI() == piRenderer::API::GL || mRenderer->GetAPI() == piRenderer::API::GLES)
        {
            mRenderer->SetWriteMask(true, false, false, false, true);
            mRenderer->SetShadingSamples(1);
            mRenderer->SetState(piSTATE_VIEWPORT_FLIPY, false);
            mRenderer->SetState(piSTATE_BLEND, false);
            mRenderer->SetState(piSTATE_DEPTH_TEST, true);
            mRenderer->SetState(piSTATE_DEPTH_CLAMP, true);
            mRenderer->SetState(piSTATE_FRONT_FACE, true);
            mRenderer->SetState(piSTATE_CULL_FACE, true);
#if CUSTOM_ALPHA_TO_COVERAGE!=1
            mRenderer->SetState(piSTATE_ALPHA_TO_COVERAGE, mRenderer->GetAPI() == piRenderer::API::GLES);
#endif
            mRenderer->SetState(piSTATE_DEPTH_TEST, true);
        }
        else
        {
            mRenderer->SetDepthState(mDepthState);
            mRenderer->SetRasterState(mRasterState);
            mRenderer->SetBlendState(mBlendState);
        }

        mLayerPaintRender->DisplayRender(mRenderer, mLog, mLayerStateShaderConstans, mDeltaCap);
        mLayerRenderPicture.DisplayRender(mRenderer, mLog, mLayerStateShaderConstans, mDeltaCap);
        mLayerRenderSound.DisplayRender(mRenderer, mLog, mLayerStateShaderConstans, mDeltaCap);
        mLayerRenderModel.DisplayRender(mRenderer, mLog, mLayerStateShaderConstans, mDeltaCap);

        if (mRenderer->GetAPI() == piRenderer::API::GL || mRenderer->GetAPI() == piRenderer::API::GLES)
        {
            mRenderer->SetState(piSTATE_BLEND, false);
            mRenderer->SetState(piSTATE_DEPTH_TEST, true);
        }

        PopulateDisplayRenderPerfInfo();

        #if defined(RENDER_BUDGET) || defined(MEASURE_GPU_TIME)
        if (mEnablePerformanceMeasurement)
        {
            mRenderer->EndPerformanceMeasure();
            mMicrosecondsLastFrame = static_cast<uint32_t>(mRenderer->GetPerformanceMeasure() /
                                                           1000ULL);
        }
        #endif
    }

    void Player::RenderStereoMultiPass(const ivec2 & pixelResolutionIncludingSupersampling, int eyeID, const mat4x4d & head_to_eye, const mat4x4 & projection)
    {
        if (!mEnabled) return;
        if (!mAnyDocToRender) return;

        #if defined(RENDER_BUDGET) || defined(MEASURE_GPU_TIME)
        if (mEnablePerformanceMeasurement)
            mRenderer->StartPerformanceMeasure();
        #endif

        mRenderer->AttachShaderConstants(mFrameStateShaderConstans, 0);
        mRenderer->AttachShaderConstants(mLayerStateShaderConstans, 3);
        mRenderer->AttachShaderConstants(mDisplayStateShaderConstans, 4);
        mRenderer->AttachShaderConstants(mPassStateShaderConstans, 5);
        mRenderer->AttachShaderConstants(mGlobalResourcesConstans, 7);


        const int eid = (mRenderer->GetAPI() == piRenderer::API::GL || mRenderer->GetAPI() == piRenderer::API::GLES) ? eyeID : 0;

        mDisplayRenderState.mEye[eid].mViewerToEye_Prj = iConvertProjectionMatrix(projection) * d2f(head_to_eye);
        mDisplayRenderState.mResolution = vec2(float(pixelResolutionIncludingSupersampling.x), float(pixelResolutionIncludingSupersampling.y));

        mRenderer->UpdateBuffer(mDisplayStateShaderConstans, &mDisplayRenderState, 0, sizeof(mDisplayRenderState));

        mPassState.mID = eyeID;
        mRenderer->UpdateBuffer(mPassStateShaderConstans, &mPassState, 0, sizeof(PassState));

        //=======================================================================================================
        // render
        //=======================================================================================================

        if (mRenderer->GetAPI() == piRenderer::API::GL || mRenderer->GetAPI() == piRenderer::API::GLES)
        {
            mRenderer->SetWriteMask(true, false, false, false, true);
            mRenderer->SetShadingSamples(1);
            mRenderer->SetState(piSTATE_VIEWPORT_FLIPY, false);
            mRenderer->SetState(piSTATE_BLEND, false);
            mRenderer->SetState(piSTATE_DEPTH_TEST, true);
            mRenderer->SetState(piSTATE_DEPTH_CLAMP, true);
            mRenderer->SetState(piSTATE_FRONT_FACE, true);
            mRenderer->SetState(piSTATE_CULL_FACE, true);
#if CUSTOM_ALPHA_TO_COVERAGE!=1
            mRenderer->SetState(piSTATE_ALPHA_TO_COVERAGE, mRenderer->GetAPI() == piRenderer::API::GLES);
#endif
            mRenderer->SetWriteMask(true, false, false, false, true);
        }
        else
        {
            mRenderer->SetDepthState(mDepthState);
            mRenderer->SetRasterState(mRasterState);
            mRenderer->SetBlendState(mBlendState);
        }

        mLayerPaintRender->DisplayRender(mRenderer, mLog, mLayerStateShaderConstans, mDeltaCap);
        mLayerRenderPicture.DisplayRender(mRenderer, mLog, mLayerStateShaderConstans, mDeltaCap);
        mLayerRenderSound.DisplayRender(mRenderer, mLog, mLayerStateShaderConstans, mDeltaCap);
        mLayerRenderModel.DisplayRender(mRenderer, mLog, mLayerStateShaderConstans, mDeltaCap);

        if (mRenderer->GetAPI() == piRenderer::API::GL || mRenderer->GetAPI() == piRenderer::API::GLES)
        {
            mRenderer->SetState(piSTATE_BLEND, false);
            mRenderer->SetState(piSTATE_DEPTH_TEST, true);
        }

        #if defined(RENDER_BUDGET) || defined(MEASURE_GPU_TIME)
        if (mEnablePerformanceMeasurement)
        {
            mRenderer->EndPerformanceMeasure();
            mMicrosecondsLastFrame = static_cast<uint32_t>(mRenderer->GetPerformanceMeasure() /
                                                           1000ULL);
        }
        #endif

        PopulateDisplayRenderPerfInfo();
    }

    void Player::RenderStereoSinglePass(const ivec2 & pixelResolutionIncludingSupersampling, const mat4x4d & head_to_lEye, const mat4x4 & lProjection, const mat4x4d & head_to_rEye, const mat4x4 & rProjection)
    {
        if (!mEnabled) return;
        if (!mAnyDocToRender) return;

        #if defined(RENDER_BUDGET) || defined(MEASURE_GPU_TIME)
        if (mEnablePerformanceMeasurement)
            mRenderer->StartPerformanceMeasure();
        #endif

        mRenderer->AttachShaderConstants(mFrameStateShaderConstans, 0);
        mRenderer->AttachShaderConstants(mLayerStateShaderConstans, 3);
        mRenderer->AttachShaderConstants(mDisplayStateShaderConstans, 4);
        mRenderer->AttachShaderConstants(mPassStateShaderConstans, 5);
        mRenderer->AttachShaderConstants(mGlobalResourcesConstans, 7);

        mDisplayRenderState.mEye[0].mViewerToEye_Prj = iConvertProjectionMatrix(lProjection) * d2f(head_to_lEye);
        mDisplayRenderState.mEye[1].mViewerToEye_Prj = iConvertProjectionMatrix(rProjection) * d2f(head_to_rEye);
        mDisplayRenderState.mResolution = vec2(float(pixelResolutionIncludingSupersampling.x), float(pixelResolutionIncludingSupersampling.y));

        mRenderer->UpdateBuffer(mDisplayStateShaderConstans, &mDisplayRenderState, 0, sizeof(mDisplayRenderState));

        //=======================================================================================================
        // render
        //=======================================================================================================

        if (mRenderer->GetAPI() == piRenderer::API::GL || mRenderer->GetAPI() == piRenderer::API::GLES)
        {
            mRenderer->SetWriteMask(true, false, false, false, true);
            mRenderer->SetShadingSamples(1);
            mRenderer->SetState(piSTATE_VIEWPORT_FLIPY, false);
            mRenderer->SetState(piSTATE_BLEND, false);
            mRenderer->SetState(piSTATE_DEPTH_TEST, true);
            mRenderer->SetState(piSTATE_DEPTH_CLAMP, true);
            mRenderer->SetState(piSTATE_FRONT_FACE, true);
            mRenderer->SetState(piSTATE_CULL_FACE, true);
#if CUSTOM_ALPHA_TO_COVERAGE!=1
            mRenderer->SetState(piSTATE_ALPHA_TO_COVERAGE, mRenderer->GetAPI() == piRenderer::API::GLES);
#endif
        }
        else
        {
            mRenderer->SetDepthState(mDepthState);
            mRenderer->SetRasterState(mRasterState);
            mRenderer->SetBlendState(mBlendState);
        }

        mLayerPaintRender->DisplayRender(mRenderer, mLog, mLayerStateShaderConstans, mDeltaCap);
        mLayerRenderPicture.DisplayRender(mRenderer, mLog, mLayerStateShaderConstans, mDeltaCap);
        mLayerRenderSound.DisplayRender(mRenderer, mLog, mLayerStateShaderConstans, mDeltaCap);
        mLayerRenderModel.DisplayRender(mRenderer, mLog, mLayerStateShaderConstans, mDeltaCap);

        if (mRenderer->GetAPI() == piRenderer::API::GL || mRenderer->GetAPI() == piRenderer::API::GLES)
        {
            mRenderer->SetState(piSTATE_BLEND, false);
            mRenderer->SetState(piSTATE_DEPTH_TEST, true);
        }

        #if defined(RENDER_BUDGET) || defined(MEASURE_GPU_TIME)
        if (mEnablePerformanceMeasurement)
        {
            mRenderer->EndPerformanceMeasure();
            mMicrosecondsLastFrame = static_cast<uint32_t>(mRenderer->GetPerformanceMeasure() /
                                                           1000ULL);
        }
        #endif

        PopulateDisplayRenderPerfInfo();
    }

    int Player::Load(const wchar_t* name)
    {
        if (piwstrlen(name) == 0)
            return -1;
        const wchar_t* ext = piFileName_GetExtension(name);
        if (piwstrcmp(ext, L".imm") != 0)
            return -1;

        mCPULoadStartTimeMS = std::chrono::system_clock::now();

        //mCommand.mArguments.CopyW(name);

        uint64_t id;
        bool isNew;
        Document *doc = (Document *)mDocuments.Alloc(&isNew, &id, true);
        if (!doc)
            return -1;

        new (doc) Document();

        if (!doc->Init(name, static_cast<uint32_t>(id)) )
            return -1;

        int cmdId = static_cast<int>(mCommandList.GetLength());
        Player::Command* newCommand = mCommandList.New(1, false);
        newCommand->mTarget = static_cast<int>(id);
        newCommand->mCommand.mType = Document::Command::Type::Load;

        newCommand->mCommand.mFileType = Document::ImportType::IMM_disk;
        if (!newCommand->mCommand.mStrArg.InitCopyW(name)) return -1;
        mSynced[id] = false;
        doc->SetCommandId(cmdId);
        return static_cast<int>(id);
    }

    int Player::Load(piTArray<uint8_t>* imm, const wchar_t* name)
    {
        mCPULoadStartTimeMS = std::chrono::system_clock::now();

        uint64_t id;
        bool isNew;
        mDocuments.Alloc(&isNew, &id, true);
        Document *doc = (Document *)mDocuments.GetAddress(id);
        if (!doc)
            return false;

        new (doc) Document();

        if (!doc->Init(name, static_cast<uint32_t>(id)))
            return false;
        int cmdId = static_cast<int>(mCommandList.GetLength());
        Player::Command* newCommand = mCommandList.New(1, false);
        newCommand->mTarget = static_cast<int>(id);
        newCommand->mCommand.mType = Document::Command::Type::Load;
        newCommand->mCommand.mArrayArg = imm;
        newCommand->mCommand.mFileType = Document::ImportType::IMM_memory;
        mSynced[id] = false;
        doc->SetCommandId(cmdId);
        return static_cast<int>(id);
    }

    void Player::Unload(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        piAssert(doc != nullptr);

        mLog->Printf(LT_MESSAGE, L"Unload(%d) received", id);
        int cmdId = doc->GetCommandId();
        mCommandList[cmdId].mCommand.mType = Document::Command::Type::Unload;
        mCommandList[cmdId].mTarget = id;
        mSynced[id] = false;

    }

    void Player::UnloadAll()
    {
        for (int i = 0; i < mDocuments.GetMaxLength(); i++)
        {
            if (!mDocuments.IsUsed(i)) continue;
            Document *doc = (Document *)mDocuments.GetAddress(i);
            int cmdId = doc->GetCommandId();
            mCommandList[cmdId].mCommand.mType = Document::Command::Type::Unload;
            mCommandList[cmdId].mTarget = i;
            mSynced[i] = false;
        }
    }

    void Player::UnloadAllSync()
    {
        const uint64_t numDocs = mDocuments.GetMaxLength();
        int numUsed = 0;

        for (int i = 0; i < numDocs; i++)
        {
            if (!mDocuments.IsUsed(i)) continue;
            numUsed++;
            Document *doc = (Document *)mDocuments.GetAddress(i);
            int cmdId = doc->GetCommandId();
            mCommandList[cmdId].mCommand.mType = Document::Command::Type::Unload;
            mCommandList[cmdId].mTarget = i;
            mSynced[i] = false;
        }

        if (numUsed == 0) return;

        bool allUnloaded = false;
        while (allUnloaded == false)
        {
            GlobalWork(true, 9000);
            GlobalRender(mViewerInfo.mVRToHead, mViewerInfo.mWorldToHead, mViewerInfo.mProjection, StereoMode::None);

            allUnloaded = true;
            for (int i = 0; i < numDocs; i++)
            {
                if (!mDocuments.IsUsed(i)) continue;
                Document *doc = (Document *)mDocuments.GetAddress(i);
                if (doc->GetLoadingState() != Document::LoadingState::UnloadingCompleted)
                {
                    allUnloaded = false;
                    break;
                }
            }
        }
    }





    void Player::SetTime(int id, piTick timeSinceStart, piTick timeSinceStop)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        doc->SetTime(mTime, timeSinceStart, timeSinceStop);
    }

    void Player::GetTime(int id, piTick * timeSinceStart, piTick * timeSinceStop)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        doc->GetTime(mTime, timeSinceStart, timeSinceStop);
    }

    int Player::GetLoadTimeInMs()
    {
        auto end = std::chrono::system_clock::now();
        return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(end - mCPULoadStartTimeMS).count());
    }

    void Player::Pause(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        int cmdId = doc->GetCommandId();
        mCommandList[cmdId].mCommand.mType = Document::Command::Type::Pause;
        mCommandList[cmdId].mTarget = id;
    }

    void Player::Pause(int id, uint64_t stopTicks)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        int cmdId = doc->GetCommandId();
        mCommandList[cmdId].mCommand.mType = Document::Command::Type::Pause;
        mCommandList[cmdId].mTarget = id;
        mCommandList[cmdId].mCommand.mIntArg = stopTicks;
    }

    void Player::Hide(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        int cmdId = doc->GetCommandId();
        mCommandList[cmdId].mCommand.mType = Document::Command::Type::Hide;
        mCommandList[cmdId].mTarget = id;
    }
    void Player::Show(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        int cmdId = doc->GetCommandId();
        mCommandList[cmdId].mCommand.mType = Document::Command::Type::Show;
        mCommandList[cmdId].mTarget = id;
    }
    void Player::Resume(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        int cmdId = doc->GetCommandId();
        mCommandList[cmdId].mCommand.mType = Document::Command::Type::Resume;
        mCommandList[cmdId].mTarget = cmdId;
    }

    void Player::Resume(int id, uint64_t startTicks)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        int cmdId = doc->GetCommandId();
        mCommandList[cmdId].mCommand.mType = Document::Command::Type::Resume;
        mCommandList[cmdId].mTarget = id;
        mCommandList[cmdId].mCommand.mIntArg = startTicks;
    }

    void Player::SkipForward(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        int cmdId = doc->GetCommandId();
        mCommandList[cmdId].mCommand.mType = Document::Command::Type::SkipForward;
        mCommandList[cmdId].mTarget = id;
    }
    void Player::SkipBack(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        int cmdId = doc->GetCommandId();
        mCommandList[cmdId].mCommand.mType = Document::Command::Type::SkipBack;
        mCommandList[cmdId].mTarget = id;
    }
    void Player::Restart(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        int cmdId = doc->GetCommandId();
        mCommandList[cmdId].mCommand.mType = Document::Command::Type::Restart;
        mCommandList[cmdId].mTarget = id;
    }
    void Player::Continue(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        int cmdId = doc->GetCommandId();
        mCommandList[cmdId].mCommand.mType = Document::Command::Type::Continue;
        mCommandList[cmdId].mTarget = id;
    }

    int Player::GetChapterCount(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        return doc->GetChapterCount();
    }

    int Player::GetCurrentChapter(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        return doc->GetCurrentChapter();
    }

    bool Player::GetHasAudio(int id)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        return doc->GetHasAudio();
    }

    float Player::GetDocumentVolume(int id) const
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        return doc->GetVolume();
    }

    void Player::SetDocumentVolume(int id, float volume)
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        doc->SetVolume(volume, mLog);
    }

    bound3d Player::GetDocumentBBox(int id) const
    {
        Document *doc = (Document *)mDocuments.GetAddress(id);
        return doc->GetBBox();
    }
}
