#if defined(ANDROID)
#define AA 4
#include "android/log.h"
#else
#define AA 8
#endif


// if you enable the avatars, disable _controlfp(_EM_UNDERFLOW | _EM_INEXACT, _MCW_EM); in mymain.cpp (line 147)
#include <algorithm>
#include <thread>

#include "../settings.h"
#include "piCameraD.h"
#include "viewer.h"

using namespace ImmCore;
using namespace ImmImporter;
using namespace ImmPlayer;

namespace ExePlayer
{

bool Viewer::Init(const char *appID_DELETE_ME_THIS_IS_A_HACK, piRenderer* renderer, piSoundEngine* soundEngine, piLog *log, piTimer *timer, StereoMode stereMode, Settings* playerSettings)
{
    for (int i = 0; i < 4; i++) mDocuments[i] = -1;

    if (playerSettings->mPlayback.mPlayerSpawn.mLocation.EqualW(L"Default"))        mCamera.SetWorldToCamera(mat4x4d::identity());
    else if (playerSettings->mPlayback.mPlayerSpawn.mLocation.EqualW(L"Custom"))         mCamera.SetWorldToCamera(invert(toMatrix(playerSettings->mPlayback.mPlayerSpawn.mCustom)));
    else { log->Printf(LT_ERROR, L"Unknown spawn position"); return false; }


    mStereoMode = stereMode;
    mLog = log;
    mRenderer = renderer;
    mSoundEngine = soundEngine;
    mPlayerSettings = playerSettings;
    mViewDurationInMs = -1;

    Player::Configuration conf;
    conf.colorSpace = mPlayerSettings->mRendering.mRenderingAPI == Settings::Rendering::API::GLES ?
        Drawing::ColorSpace::Gamma : Drawing::ColorSpace::Linear;
    conf.multisamplingLevel = AA;
    conf.depthBuffer = DepthBuffer::Linear01;
    conf.clipDepth = (mPlayerSettings->mRendering.mRenderingAPI == Settings::Rendering::API::GL || mPlayerSettings->mRendering.mRenderingAPI == Settings::Rendering::API::GLES) ?
        ClipSpaceDepth::FromNegativeOneToOne : ClipSpaceDepth::FromZeroToOne;
    conf.projectionMatrix = ClipSpaceDepth::FromNegativeOneToOne;
    conf.frontIsCCW = true;
    conf.paintRenderingTechnique = static_cast<Drawing::PaintRenderingTechnique>(mPlayerSettings->mRendering.mRenderingTechnique);

    if (!mPlayer.Init(mRenderer, mSoundEngine, mLog, timer, &conf))
        return false;
    //-----------------

    mDisplayStateShaderConstans = mRenderer->CreateBuffer(nullptr, sizeof(DisplayRenderState), piRenderer::BufferType::Dynamic, piRenderer::BufferUse::Constant);
    if (!mDisplayStateShaderConstans) return false;

    //-----------------
    // default spawn area
    mCurrentSpawnAreaID = 0;
    mCurrentSpawnArea = Document::SpawnAreaInfo{ L"SpacesSpawnArea", 0, false,  trans3d::identity(),
    LayerSpawnArea::Volume{LayerSpawnArea::Volume::Type::Sphere, LayerSpawnArea::Volume::Shape{vec4(0.0f, 1.0f, 0.0f, 2.0f)}, true, true, true},false };

    // LOADING
    mFilesLoaded = false;
    for (int i = 0; i < mPlayerSettings->mFiles.mLoad.GetLength(); i++)
    {
        mDocuments[i] = mPlayer.Load(mPlayerSettings->mFiles.mLoad[i].GetS());
        if (mDocuments[i] != -1)
        {
            mPlayer.SetDocumentToWorld(mDocuments[i], mPlayerSettings->mPlayback.mLocation);
            mFilesLoaded = true;
        }
        else
        {
            mLog->Printf( LT_ERROR, L"Couldn't load file \"%s\"",mPlayerSettings->mFiles.mLoad[i].GetS() );
        }
    }

    mInitialized = true;

    return true;
}

void Viewer::Deinit(void)
{
    if (!mInitialized)
        return;
    mPlayer.UnloadAllSync();
    mPlayer.Deinit();

    mRenderer->DestroyBuffer(mDisplayStateShaderConstans);
    if(mChaptersViewed.IsInitialized())
        mChaptersViewed.End();

    mInitialized = false;
}


static trans3d iGetControllerLocation(const piVRHMD::Controller* ctrl)
{
    return trans3d(quatd(double(ctrl->mRotation[0]),
        double(ctrl->mRotation[1]),
        double(ctrl->mRotation[2]),
        double(ctrl->mRotation[3])), 1.0, flip3::N,
        vec3d(double(ctrl->mPosition[0]),
            double(ctrl->mPosition[1]),
            double(ctrl->mPosition[2])));
}

void Viewer::iHandleGlobalInputs(piWindowEvents* evt, float dtime, const ivec2 & wndowSize)
{
    double speed = 1.0;
    vec3d dis = vec3d(0.0, 0.0, 0.0);
    if (evt->keyb.state['A']) { dis.x -= 1.0; }
    if (evt->keyb.state['D']) { dis.x += 1.0; }
    if (evt->keyb.state['E']) { dis.y += 1.0; }
    if (evt->keyb.state['Q']) { dis.y -= 1.0; }
    if (evt->keyb.state['W']) { dis.z -= 1.0; }
    if (evt->keyb.state['S']) { dis.z += 1.0; }
    if (evt->keyb.state[KEY_SHIFT]) { speed *= 5.0; }
    if (evt->keyb.state[KEY_CONTROL]) { speed /= 5.0; }
    if (evt->mouse.lb_isDown)
    {
        const vec2d mxy = double(dtime) * 1500.0 * vec2d(double(evt->mouse.dx) / double(wndowSize.x), double(evt->mouse.dy) / double(wndowSize.y));
        mCamera.RotateXY(mxy.x, mxy.y);
    }
    mCamera.LocalMove(dtime * speed * dis);

#if 0    // enable only for developers to test stuff
    if (evt->keyb.key['b']) { enabled = !enabled; mLog->Printf(LT_DEBUG, (enabled) ? L"ENABLE" : L"DISABLE"); evt->keyb.key['b'] = 0; }
    if (evt->keyb.key['h']) { if (state.mPlaybackState == Player::PlaybackState::PausedAndHidden) { mLog->Printf(LT_DEBUG, L"SHOW!"); mPlayer->Show(mDocuments[0]); } else { mLog.Printf(LT_DEBUG, L"HIDE!"); mPlayer->Hide(mDocuments[0]); } evt->keyb.key['p'] = 0; }
    if (evt->keyb.key['k']) { mPlayer->GetTime(mDocuments[0], &saveTimeTest.timeSinceStart, &saveTimeTest.timeSinceStop); mLog->Printf(LT_DEBUG, L"SAVE TIMEPOINT start: %lld  stop: %lld", saveTimeTest.timeSinceStart, saveTimeTest.timeSinceStop); evt->keyb.key['k'] = 0; }
    if (evt->keyb.key['j']) { mLog->Printf(LT_DEBUG, L"JUMP TO TIMEPOINT start: %lld  stop: %lld", saveTimeTest.timeSinceStart, saveTimeTest.timeSinceStop); mPlayer->SetTime(mDocuments[0], saveTimeTest.timeSinceStart, saveTimeTest.timeSinceStop); evt->keyb.key['j'] = 0; }
#endif

}

void Viewer::iHandleDocumentInputs(piWindowEvents *evt,
    const bool vrEnabled,
    const piVRHMD::Controller *controller, // two controllers, [0]=left, [1]=right
    const piVRHMD::Remote     *remote,
    const Player::DocumentState & state)
{
    // Jump to SpawnArea
    for (int i = 0; i < 10; i++)
    {
        const char keychar = "!@#$%^&*()"[i];

        if (evt->keyb.key[keychar])
        {
            mLog->Printf(LT_DEBUG, L"SpawnArea %d", i);
            SetSpawnArea(0, i, true);
            evt->keyb.key[keychar] = 0;
        }
    }

    if (evt->keyb.key['n'])
    {
        mPlayer.Unload(mDocuments[0]);
        evt->keyb.key['n'] = 0;
    }


    // Story navigation
    if (evt->keyb.key['z']) { mLog->Printf(LT_DEBUG, L"SKIP BACK!"); mPlayer.SkipBack(mDocuments[0]); evt->keyb.key['z'] = 0; }
    if (evt->keyb.key['x']) { mLog->Printf(LT_DEBUG, L"SKIP FORWARD!"); mPlayer.SkipForward(mDocuments[0]); evt->keyb.key['x'] = 0; }
    if (evt->keyb.key['c']) { mLog->Printf(LT_DEBUG, L"RESTART!"); mPlayer.Restart(mDocuments[0]); evt->keyb.key['c'] = 0; }
    if (evt->keyb.key['v']) { mLog->Printf(LT_DEBUG, L"CONTINUE!"); mPlayer.Continue(mDocuments[0]); evt->keyb.key['v'] = 0; }
    if (evt->keyb.key['p'])
    {
        if (state.mPlaybackState == Player::PlaybackState::Paused || state.mPlaybackState == Player::PlaybackState::PausedAndHidden)
        {
            mLog->Printf(LT_DEBUG, L"RESUME!");
            mPlayer.Resume(mDocuments[0]);
        }
        else
        {
            mLog->Printf(LT_DEBUG, L"PAUSE!");
            mPlayer.Pause(mDocuments[0]);

        }
        evt->keyb.key['p'] = 0;
    }

    // spawnAreas
    const int num = mPlayer.GetSpawnAreaCount(mDocuments[0]);
    for (int i = 0; i < num; i++)
    {
        Document::SpawnAreaInfo info;
        mPlayer.GetSpawnAreaInfo(info, mDocuments[0], i);
    }
}

void Viewer::SetSpawnArea(int docID, int spawnAreaID, bool recenter)
{
    int spawnAreaCount = mPlayer.GetSpawnAreaCount(docID);
    if (spawnAreaID < 0 || spawnAreaCount <= spawnAreaID)
    {
        return;
    }

    // when changing spawnAreas, resave the eye level offset
    if (recenter)
    {
        vec3d viewerDir = normalize((mViewerInfo.mViewerToVR * vec4d(0.0f, 0.0f, 1.0f, 0.0f)).xyz());
        double yRotation = atan2(viewerDir.x, viewerDir.z);
        // save position + yRotation
        mViewerInfo.mSavedViewerToVR = trans3d(quatd::rotateY(yRotation), 1.0, flip3::N, mViewerInfo.mViewerToVR.mTranslation);
    }

    mCurrentSpawnAreaID = spawnAreaID;

    // Set initial spawn area
    mPlayer.GetSpawnAreaInfo(mCurrentSpawnArea, mDocuments[docID], mCurrentSpawnAreaID);

    trans3d viewerTransform = mViewerInfo.mSavedViewerToVR;

    if (mCurrentSpawnArea.mIsFloorLevel)
    {
        viewerTransform.mTranslation.y = 0.0;
    }

    mCamera.SetWorldToCamera(toMatrix( viewerTransform * invert(mCurrentSpawnArea.mSpawnAreaToWorld) ));

    mPlayer.SetSpawnArea(docID, spawnAreaID);
}


void Viewer::GlobalWork(piWindowEvents *evt, const bool vrEnabled, const trans3d & vr_to_head, const piVRHMD::Controller *controllers, const piVRHMD::Remote *remote, piLog *log,
    float dtime, const ivec2 & wndowSize, bool enabled, uint32_t microsecondsBudget, bool& isFirstFrame)
{
    mViewerInfo.mWorldToVR = fromMatrix(mCamera.GetWorldToCamera());
    mViewerInfo.mVRToWorld = fromMatrix(mCamera.GetCameraToWorld());
    mViewerInfo.mVRToViewer = vr_to_head;
    mViewerInfo.mViewerToVR = invert(mViewerInfo.mVRToViewer);


#if !defined(ANDROID)
    // global keyboard events
    iHandleGlobalInputs(evt, dtime, wndowSize);
#endif


    if (mDocuments[0] != -1)
    {
        mPlayer.GlobalWork(enabled ? 1 : 0, microsecondsBudget);

        Player::DocumentState state;
        mPlayer.GetDocumentState(state, mDocuments[0]);

        if (mLastDocumentLoadingState != state.mLoadingState && mOnLoadingStateChangeHandler != nullptr)
        {
            mOnLoadingStateChangeHandler(state.mLoadingState);
            mLastDocumentLoadingState = state.mLoadingState;
        }

        if (state.mLoadingState == Player::LoadingState::Loaded)
        {
            MeasureEngagement();

#if !defined(ANDROID)
            iHandleDocumentInputs(evt, vrEnabled, controllers, remote, state);
#endif

            if (isFirstFrame || mPlayer.GetSpawnAreaNeedsUpdate(mDocuments[0]) || (
                state.mPlaybackState != Player::PlaybackState::Paused && state.mPlaybackState != Player::PlaybackState::PausedAndHidden
                && mCurrentSpawnArea.mAnimated)
               )
            {
                mCurrentSpawnAreaID = std::max(mPlayer.GetSpawnArea(mDocuments[0]), 0);

                // handle spawn area change keyframes
                if (mPlayer.GetSpawnAreaNeedsUpdate(mDocuments[0]))
                {
                    mCurrentSpawnAreaID = mPlayer.GetInitialSpawnArea(mDocuments[0]);
                    mPlayer.SetSpawnAreaNeedsUpdate(mDocuments[0], false);
                }

                if (mPlayer.GetSpawnAreaCount(mDocuments[0]) > 0)
                {
                    SetSpawnArea(mDocuments[0], mCurrentSpawnAreaID, isFirstFrame);
                }

                isFirstFrame = false;
            }


        }

    }

    // update UI for loading states
    Player::PlayerInfo playerInfo;
    mPlayer.GetPlayerInfo(playerInfo);

    int numDocs = 0;
    for (int i = 0; i < 4; i++)
    {
        if (mDocuments[i] == -1) continue;

        Player::DocumentState state;
        mPlayer.GetDocumentState(state, mDocuments[i]);
        if (state.mLoadingState == Player::LoadingState::Loading || state.mLoadingState == Player::LoadingState::Unloading)
        {
            numDocs++;
        }
    }
}

static const float kNear = 0.01f;
static const float kFar = 20000.0f; // 20 kilometers. This is how far you can see on the surface of earth before planer curvature hides things from you
static const float kFOV = 35.0f; // for mono only

void Viewer::GlobalRender(const trans3d & vr_to_head, const vec4 & projection)
{
    mat4x4 prj;
    if (mStereoMode == StereoMode::None)
    {
        const float aspect = 16.0f / 9.0f;
        const float tan = ::tanf(kFOV * 3.141592653589f / 180.0f);
        const float x = 1.0f / (tan*aspect);
        const float y = 1.0f / (tan);
        //const float c = -(zfar + kNear) / (kFar - kNear);
        //const float d = -(2.0f * kFar * kNear) / (zfar - kNear);
        const float c = -kFar / (kFar - kNear);
        const float d = -(kFar * kNear) / (kFar - kNear);
        prj = mat4x4(x, 0.0f, 0.0f, 0.0f, 0.0f, y, 0.0f, 0.0f, 0.0f, 0.0f, c, d, 0.0f, 0.0f, -1.0f, 0.0f);
    }
    else
    {
        prj = setProjection(projection, float(kNear), float(kFar));
    }
    GlobalRender(vr_to_head, prj);
}

void Viewer::GlobalRender(const trans3d & vr_to_head, const mat4x4 & projection)
{
    const trans3d world_to_head = vr_to_head * fromMatrix(mCamera.GetWorldToCamera());

    mPlayer.GlobalRender(vr_to_head, world_to_head, projection, mStereoMode);

    // TODO remove hardcoded docID of 0 in future
    if (GetDocumentLoadState(0) == Player::LoadingState::Loaded) {
        AccumulateViewTime();
    }
}

void Viewer::RenderStereoSinglePass(const ivec2 & pixelResolutionIncludingSupersampling,
    const trans3d & vr_to_head,
    const mat4x4d headToLEye, const vec4 & projectionLeft,
    const mat4x4d headToREye, const vec4 & projectionRight,
    piVRHMD* hmd)
{
    const mat4x4 lProjection = setProjection(vec4(hmd->mInfo.mEye[0].mProjection), kNear, kFar);
    const mat4x4 rProjection = setProjection(vec4(hmd->mInfo.mEye[1].mProjection), kNear, kFar);

    RenderStereoSinglePass(pixelResolutionIncludingSupersampling, vr_to_head, headToLEye, lProjection, headToREye, rProjection, hmd);
}

void Viewer::RenderStereoSinglePass(const ivec2 & pixelResolutionIncludingSupersampling,
    const trans3d & vr_to_head,
    const mat4x4d headToLEye, const mat4x4 & lProjection,
    const mat4x4d headToREye, const mat4x4 & rProjection,
    piVRHMD* hmd)
{

    Player::PlayerInfo playerInfo;
    mPlayer.GetPlayerInfo(playerInfo);

    if( !mFilesLoaded )
    {
        // Dicovery Mode
        const float colorBackground[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        mRenderer->Clear(colorBackground, nullptr, nullptr, nullptr, true);
    }
    else
    {
        const float colorBackground[4] = { playerInfo.mBackgrundColor.mRed,  playerInfo.mBackgrundColor.mGreen,  playerInfo.mBackgrundColor.mBlue, 1.0f };
        mRenderer->Clear(colorBackground, nullptr, nullptr, nullptr, true);
    }

    mPlayer.RenderStereoSinglePass(pixelResolutionIncludingSupersampling, headToLEye, lProjection, headToREye, rProjection);

    mRenderer->SetState(piSTATE_BLEND, false);
    mRenderer->SetState(piSTATE_DEPTH_TEST, true);


    const trans3d world_to_head = vr_to_head * fromMatrix(mCamera.GetWorldToCamera());
}

void Viewer::RenderStereoMultiPass(const ivec2 & pixelResolutionIncludingSupersampling,
    int passId,
    const mat4x4d headToEye, const vec4 & eyeProjection,
    const trans3d vr_to_head)
{
    mat4x4 proj = setProjection(eyeProjection, kNear, kFar);
    RenderStereoMultiPass(pixelResolutionIncludingSupersampling, passId, headToEye, proj, vr_to_head);
}
void Viewer::RenderStereoMultiPass(const ivec2 & pixelResolutionIncludingSupersampling,
    int passId,
    const mat4x4d headToEye, const mat4x4 & eyeProjection,
    const trans3d vr_to_head)
{
    Player::PlayerInfo playerInfo;
    mPlayer.GetPlayerInfo(playerInfo);
    if (mFilesLoaded == false)
    {
        const float colorBackground[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        mRenderer->Clear(colorBackground, nullptr, nullptr, nullptr, true);
    }
    else
    {
        const float colorBackground[4] = { playerInfo.mBackgrundColor.mRed,  playerInfo.mBackgrundColor.mGreen,  playerInfo.mBackgrundColor.mBlue, 1.0f };
        mRenderer->Clear(colorBackground, nullptr, nullptr, nullptr, true);

    }
    const trans3d world_to_head = vr_to_head * fromMatrix(mCamera.GetWorldToCamera());
    mPlayer.RenderStereoMultiPass(pixelResolutionIncludingSupersampling, passId, headToEye, eyeProjection);
}

void Viewer::RenderMono(const ivec2 & pixelResolutionIncludingSupersampling, const trans3d & vr_to_head, int eyeID)
{
    const trans3d world_to_head = vr_to_head * fromMatrix(mCamera.GetWorldToCamera());

    Player::PlayerInfo playerInfo;
    mPlayer.GetPlayerInfo(playerInfo);

    const float colorBackground[4] = { playerInfo.mBackgrundColor.mRed,  playerInfo.mBackgrundColor.mGreen,  playerInfo.mBackgrundColor.mBlue, 1.0f };
    mRenderer->Clear(colorBackground, nullptr, nullptr, nullptr, true);
    mPlayer.RenderMono(pixelResolutionIncludingSupersampling, eyeID);

    //======================================================
    // Render UI
    //======================================================
    // ...
}

bool Viewer::IsDocumentLoaded(int docID) const
{
    if (!mInitialized)
        return false;

    ImmPlayer::Player::DocumentState docState;
    mPlayer.GetDocumentState(docState, docID);
    return docState.mLoadingState == ImmPlayer::Player::LoadingState::Loaded;
}

bool Viewer::IsDocumentLoading(int docID) const
{
    if (!mInitialized)
        return false;

    Player::DocumentState docState;
    mPlayer.GetDocumentState(docState, docID);
    return docState.mLoadingState == Player::LoadingState::Loading;
}

bool Viewer::DidDocumentLoadFail(int docID) const
{
    if (!mInitialized)
        return false;

    Player::DocumentState docState;
    mPlayer.GetDocumentState(docState, docID);
    return docState.mLoadingState == Player::Player::LoadingState::Failed;
}

Player::LoadingState Viewer::GetDocumentLoadState(int docID) const
{
    Player::DocumentState docState;
    mPlayer.GetDocumentState(docState, docID);
    return docState.mLoadingState;
}

bool Viewer::ReplaceLoadingStateChangeListener(Viewer::OnDocumentStateChange handler)
{
    mOnLoadingStateChangeHandler = handler;
    return mOnLoadingStateChangeHandler != nullptr;
}

void Viewer::Unload(int docID)
{
    if (!mInitialized)
        return;

    mPlayer.Unload(docID);
}

void Viewer::UnloadAll()
{
    if (!mInitialized)
        return;

    mPlayer.UnloadAll();
}

void Viewer::UnloadAllSync()
{
    if (!mInitialized)
        return;

    mPlayer.UnloadAllSync();
}

const Player::PerformanceInfo & Viewer::GetPerformanceInfoForFrame()
{
    return mPlayer.GetPerformanceInfoForFrame();
}

void Viewer::MeasureEngagement()
{
    // TODO: in future, remove hardcoded docID of 0
    const int docID = 0;
    const int currentChapter = static_cast<int>(GetCurrentChapter(docID));

    if (!mChaptersViewed.IsInitialized())
    {
        mChaptersViewed.Init(GetChapterCount(docID), true);
    }

    // init chapter counting
    if (mLastChapter == -1)
    {
        mLastChapter = currentChapter;
    }

    // Count chapters
    if (mLastChapter != currentChapter)
    {
        mChaptersViewed.Set(true, mLastChapter);
        mLastChapter = currentChapter;
    }

    // Always count the last chapter, because it will never transition
    if (!mChaptersViewed.Get(currentChapter) && currentChapter == (GetChapterCount(docID) - 1))
    {
        mChaptersViewed.Set(true, currentChapter);
    }
}

void Viewer::AccumulateViewTime()
{
    // TODO remove hardcoded docID of 0
    bool isPlaying = !IsPaused(0);
    if (isPlaying)
    {
        auto end = std::chrono::system_clock::now();
        // first run
        if (mViewDurationInMs == -1)
        {
            mLastViewTime = end;
            mViewDurationInMs = 0;
        }
        mViewDurationInMs += static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(end - mLastViewTime).count());
        mLastViewTime = end;
    }
    //mLog->Printf(LT_DEBUG, L"harleym: view time %i", GetViewTimeInMs());
}

int Viewer::GetLoadTimeInMs()
{
    return mPlayer.GetLoadTimeInMs();
}

long Viewer::GetViewTimeInMs()
{
    return std::max<long>(mViewDurationInMs, 0);
}

int Viewer::GetChapterViewCount()
{
    int count = 0;
    for (int i = 0; i < mChaptersViewed.GetMaxLength(); i++) {
        if (mChaptersViewed.Get(i))
            count++;
    }
    return count;
}


const piImage * Viewer::GetSpawnAreaScreenshot(int docID, int spawnAreaID)
{
    int currentSpawnArea = std::max(mPlayer.GetSpawnArea(docID), 0);
    int totalSpawnArea = mPlayer.GetSpawnAreaCount(docID);

    if (totalSpawnArea == 0)
    {
        return nullptr;
    }
    return mPlayer.GetSpawnAreaScreenshot(docID, spawnAreaID);
}

int Viewer::GetInitalSpawnArea(int docID)
{
    return mPlayer.GetInitialSpawnArea(docID);
}

bool Viewer::IsPaused(int docID) const
{
    Player::DocumentState state;
    mPlayer.GetDocumentState(state, docID);

    return state.mPlaybackState == Player::PlaybackState::Paused;
}

bool Viewer::IsWaiting(int docID) const
{
    Player::DocumentState state;
    mPlayer.GetDocumentState(state, docID);

    return state.mPlaybackState == Player::PlaybackState::Waiting;
}

void Viewer::TogglePlaybackState(int docID)
{
    Player::DocumentState state;
    mPlayer.GetDocumentState(state, docID);

    if (state.mPlaybackState == Player::PlaybackState::Playing || state.mPlaybackState == Player::PlaybackState::Waiting)
    {
        mPlayer.Pause(docID);
    }
    else
    {
        mPlayer.Resume(docID);
    }
}

bool Viewer::GetVolume(int docID)
{
    return mPlayer.GetDocumentVolume(docID);
}

void Viewer::SetVolume(int docID, float volume)
{
    mPlayer.SetDocumentVolume(docID, volume);
}

bool Viewer::HasPrev(int docID)
{
    size_t currentChapter = GetCurrentChapter(docID);
    return currentChapter > 0;
}

void Viewer::Prev(int docID)
{
    if (!HasPrev(docID))
        return;

    mPlayer.SkipBack(docID);
    mLastChapter = -1;
}

bool Viewer::HasNext(int docID)
{
    size_t currentChapter = GetCurrentChapter(docID);
    size_t chapterCount = GetChapterCount(docID);
    return currentChapter + 1 < chapterCount;
}

void Viewer::Next(int docID)
{
    if (!HasNext(docID))
        return;

    mPlayer.SkipForward(docID);
    mLastChapter = -1;
}

ImmCore::trans3d Viewer::GetWorldToVR()
{
    return mViewerInfo.mWorldToVR;
}

void Viewer::CancelLoading(int docID) {
    mPlayer.CancelLoading(docID);
}

}
