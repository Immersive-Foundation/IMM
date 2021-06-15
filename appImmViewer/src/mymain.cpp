#define AA 8

// Set this to 1, ONLY if you need to build the viewer without the Oculus SDF installed
// This flag is NOT meant to force mono rendering. Mono rendering can always be forced
// from the config file even if the viewer is built to do VR
#define DISABLE_VR 0

#include <float.h>
#include "libCore/src/libBasics/piTimer.h"
#include "libCore/src/libBasics/piWindow.h"
#include "libCore/src/libBasics/piImage.h"
#if DISABLE_VR==0
#include "libCore/src/libVR/piVR.h"
#endif
#include "viewer/viewer.h"
#include "settings.h"
#include "resolve.h"
using namespace ImmCore;
using namespace ImmImporter;
using namespace ExePlayer;

//-----

static const char* vsMirror = ""

"layout(location = 0) in vec2 inVertex;"

"out V2FData"
"{"
"vec2 uv;"
"}vf;"

"void main()"
"{"
"vf.uv = 0.5 + 0.5*inVertex;"
"gl_Position = vec4(inVertex, 0.0, 1.0);"
"}";

static const char* fsMirror = ""

"layout(binding = 0) uniform sampler2D unTex0;"
"layout(location = 0, index = 0) out vec4 outColor;"
"layout(location = 1) uniform vec2 unRes;"

"in V2FData"
"{"
"vec2 uv;"
"}vf;"

"void main()"
"{"
"vec3 col = texture( unTex0, vec2( vf.uv.x, 0.5 + (vf.uv.y-0.5)/(unRes.x/unRes.y) ) ).xyz;"
"col = pow(col, vec3(0.4545));"
"outColor = vec4(col, 1.0);"
"}";


class MainRenderReporter : public piRenderer::piReporter
{
private:
    piLog* mLog;
    wchar_t tmp[2048];

public:
    MainRenderReporter(piLog* log) : piRenderer::piReporter() { mLog = log; }
    virtual ~MainRenderReporter() {}
    void Info(const char* str) { pistr2ws(tmp, 2048, str); mLog->Printf(LT_MESSAGE, L"%s", tmp); }
    void Error(const char* str, int level) { pistr2ws(tmp, 2048, str);  mLog->Printf(LT_ERROR, L"%s", tmp); }
    void Begin(uint64_t memCurrent, uint64_t memPeak, int texCurrent, int texPeak)
    {
        mLog->Printf(LT_MESSAGE, L"---- Renderer Report ---- ");
        mLog->Printf(LT_MESSAGE, L"Max Used : %d MB in %d textures and buffers", memPeak >> 20, texPeak);
        mLog->Printf(LT_MESSAGE, L"Leaked   : %d MB in %d textures and buffer", memCurrent >> 20, texCurrent);
    }
    void End(void)
    {
        mLog->Printf(LT_MESSAGE, L"---- Renderer Report ---- ");
    }
    void Texture(const wchar_t* key, uint64_t kb, piRenderer::Format format, bool compressed, int xres, int yres, int zres)
    {
        mLog->Printf(LT_MESSAGE, L"* Texture: %5d kb, %4d x %4d x %4d %2d (%s)", (int)kb, xres, yres, zres, format, (key == nullptr) ? L"null" : key);
    }
};

//----------------------------------------------------------------------------------

#if !defined(ANDROID)
extern "C" _declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
#endif


int piMainFunc(const wchar_t* path, const wchar_t** args, int numArgs, void* instance)
{
    ExePlayer::Viewer mViewer;
    piLog mLog;
    int  mSuperSample;
    piWindowMgr mWinMgr;
    piWindow mWindow;
    piRenderer* mRenderer;
    piSoundEngineBackend* mSoundEngineBackend;
    piRenderer::piReporter* mRenderReporter;
    #if DISABLE_VR==0
    piVRHMD* mHMD;
    piShader mMirrorShader;
    #endif
    ivec2 mWindowSize;
    Settings mSettings;
    piTimer       mTimer;
    ImmPlayer::StereoMode mStereoMode;
    piTexture mColorTextureM;
    piTexture mDepthTextureM;
    piRTarget mRenderTargetM;
    ivec2 mRenderSize;

    Resolve mResolve;
    #if DISABLE_VR==0
    struct TextureChain
    {
        int mRenderNumTextures;
        piTexture mRenderTexture[32];
        piRTarget mRenderTarget[32];
    } mTextureChain[2];
    #endif

#ifdef DEBUG
    //_controlfp(_EM_UNDERFLOW | _EM_INEXACT, _MCW_EM);
#endif

    if (!mLog.Init(L"debug.txt", PILOG_TXT + PILOG_CNS))
        return false;

    if (!mTimer.Init())
        return false;

    //------------------------

    const wchar_t* settingsFileName = L"settings.json";
    if (numArgs > 1)
    {
        settingsFileName = args[1];
    }
    mLog.Printf(LT_MESSAGE, L"Reading config file \"%s\"...", settingsFileName);

    if (!mSettings.Init(settingsFileName, &mLog))
        return false;

    mSuperSample = mSettings.mRendering.mSupersampling;
    if (mSuperSample < 1) { mSuperSample = 1; mLog.Printf(LT_WARNING, L"Supersampling factor must be between 1 and 3 (1 sample per pixel and 3x3 samples per pixel"); }
    if (mSuperSample > 3) { mSuperSample = 3; mLog.Printf(LT_WARNING, L"Supersampling factor must be between 1 and 3 (1 sample per pixel and 3x3 samples per pixel"); }


    mWinMgr = piWindowMgr_Init();
    if (!mWinMgr)
    {
        mSettings.End();
        return false;
    }
    mWindow = piWindow_init(mWinMgr, L"rendering", mSettings.mWindow.mPositionX, mSettings.mWindow.mPositionY, mSettings.mWindow.mWidth, mSettings.mWindow.mHeight, mSettings.mWindow.mFullScreen, !mSettings.mWindow.mFullScreen, false, mSettings.mWindow.mFullScreen);
    if (!mWindow)
    {
        mSettings.End();
        return false;
    }
    piWindow_show(mWindow);

    mLog.Printf(LT_MESSAGE, L"Rendering Backened: %s", (mSettings.mRendering.mRenderingAPI == Settings::Rendering::API::DX) ? L"DirectX" : L"OpenGL");
    mLog.Printf(LT_MESSAGE, L"Rendering Technique: %s", (mSettings.mRendering.mRenderingTechnique==Settings::Rendering::Technique::Static)?L"Static":L"Pretessellated" );
    #if DISABLE_VR==0
    mLog.Printf(LT_MESSAGE, L"Rendering in VR: %s", (mSettings.mRendering.mEnableVR) ? L"yes" : L"no");
    #else
    mLog.Printf(LT_MESSAGE, L"Rendering in VR: no" );;
    #endif

    // renderer
    mRenderReporter = new MainRenderReporter(&mLog);

    mRenderer = piRenderer::Create((mSettings.mRendering.mRenderingAPI == Settings::Rendering::API::GL) ? piRenderer::API::GL : piRenderer::API::DX);
    if (!mRenderer)
    {
        mSettings.End();
        return false;
    }

    // renderer
    const void* hwnds[1] = { piWindow_getHandle(mWindow) };
    bool disableRendererErrors = false; // can set this to true
    if (!mRenderer->Initialize(0, hwnds, 1, true, disableRendererErrors, mRenderReporter, true, nullptr))
    {
        mLog.Printf(LT_ERROR, L"Can't create renderer");
        {
            mSettings.End();
            return false;
        }
    }
    mRenderer->SetActiveWindow(0);

    //=============
    mWindowSize = ivec2(mSettings.mWindow.mWidth, mSettings.mWindow.mHeight);

    #if DISABLE_VR==0
    if (mSettings.mRendering.mEnableVR)
    {
        mHMD = nullptr;

        float pd = mSettings.mRendering.mPixelDensity;
        if (pd < 0.1f) { pd = 0.1f; mLog.Printf(LT_WARNING, L"Pixel Density too small"); }
        if (pd > 3.0f) { pd = 3.0f; mLog.Printf(LT_WARNING, L"Pixel Density too big"); }

        mHMD = piVRHMD::Create(piVRHMD::ANY_AVAILABLE, nullptr, 0, pd, &mLog, &mTimer);
        if (mHMD == nullptr)
        {
            mLog.Printf(LT_ERROR, L"Cannot do VR");
            mSettings.End();
            return false;
        }
        mStereoMode = ImmPlayer::StereoMode::Preferred;
        mRenderSize = ivec2(mHMD->mInfo.mVRXres, mHMD->mInfo.mVRYres);

        //-----------------------
        if (mHMD->mType == piVRHMD::Oculus_Rift || mHMD->mType == piVRHMD::Oculus_RiftS || mHMD->mType == piVRHMD::Oculus_Quest)
        {
            if (!mHMD->AttachToWindow(true, mWindowSize.x, mWindowSize.y))
            {
                mSettings.End();
                return false;
            }
            for (int j = 0; j < 2; j++)
            {
                TextureChain* tc = mTextureChain + j;
                tc->mRenderNumTextures = mHMD->mInfo.mTexture[j].mNum;
                for (int i = 0; i < tc->mRenderNumTextures; i++)
                {
                    tc->mRenderTexture[i] = mRenderer->CreateTextureFromID(mHMD->mInfo.mTexture[j].mTexIDColor[i], piRenderer::TextureFilter::MIPMAP);
                    tc->mRenderTarget[i] = mRenderer->CreateRenderTarget(tc->mRenderTexture[i], 0, 0, 0, 0);
                    if (!tc->mRenderTarget[i])
                    {
                        mSettings.End();
                        return false;
                    }
                }
            }
        }
        else if (mHMD->mType == piVRHMD::HTC_Vive)
        {
            for (int j = 0; j < 2; j++)
            {
                TextureChain* tc = mTextureChain + j;
                tc->mRenderNumTextures = mHMD->mInfo.mTexture[j].mNum;
                for (int i = 0; i < tc->mRenderNumTextures; i++)
                {
                    const piRenderer::TextureInfo infocm = { piRenderer::TextureType::T2D, piRenderer::Format::C4_8_UNORM_SRGB, mRenderSize.x, mRenderSize.y, 1, 1 };
                    tc->mRenderTexture[i] = mRenderer->CreateTexture(0, &infocm, false, piRenderer::TextureFilter::LINEAR, piRenderer::TextureWrap::CLAMP, 1.0f, 0);
                    if (!tc->mRenderTexture[i])
                    {
                        mSettings.End();
                        return false;
                    }
                    tc->mRenderTarget[i] = mRenderer->CreateRenderTarget(tc->mRenderTexture[i], 0, 0, 0, 0);
                    if (!tc->mRenderTarget[i])
                    {
                        mSettings.End();
                        return false;
                    }
                }
            }

            piRenderer::TextureInfo info[2];
            mRenderer->GetTextureInfo(mTextureChain[0].mRenderTexture[0], info + 0);
            mRenderer->GetTextureInfo(mTextureChain[1].mRenderTexture[0], info + 1);
            if (!mHMD->AttachToWindow2(reinterpret_cast<void*>(static_cast<uint64_t>(info[0].mDeleteMe)), reinterpret_cast<void*>(static_cast<uint64_t>(info[1].mDeleteMe))))
            {
                mSettings.End();
                return false;
            }
        }
        else
        {
            return false;
        }

        //-----------------------

        char error[2048];
        mMirrorShader = mRenderer->CreateShader(nullptr, vsMirror, nullptr, nullptr, nullptr, fsMirror, error);
        if (!mMirrorShader)
        {
            mLog.Printf(LT_ERROR, L"Can't create mirror shader");
            mSettings.End();
            return false;
        }


        if (!mRenderer->SupportsFeature(piRenderer::RendererFeature::VERTEX_VIEWPORT) || !mRenderer->SupportsFeature(piRenderer::RendererFeature::VIEWPORT_ARRAY))
        {
            mLog.Printf(LT_WARNING, L"Fast stereo is not available, falling back to slow stereo");
            mStereoMode = ImmPlayer::StereoMode::Fallback;
        }
        else
        {
            mLog.Printf(LT_MESSAGE, L"Fast stereo enabled");
            mStereoMode = ImmPlayer::StereoMode::Preferred;
        }

        mHMD->SetTrackingOriginType(piVRHMD::TrackingOrigin::FloorLevel);
    }
    else
    #endif
    {
        #if DISABLE_VR==0
        mHMD = nullptr;
        #endif
        mStereoMode = ImmPlayer::StereoMode::None;
        mRenderSize = mWindowSize;
    }

    //------------------------------

    mSoundEngineBackend = piCreateSoundEngineBackend(piSoundEngineBackend::API::DirectSoundOVR,&mLog);
    if (!mSoundEngineBackend)
    {
        mSettings.End();
        return false;
    }

    const int num = mSoundEngineBackend->GetNumDevices();
    mLog.Printf(LT_MESSAGE, L"%d sound devices", num);
    for (int i = 0; i < num; ++i)
    {
        const wchar_t * deviceName = mSoundEngineBackend->GetDeviceName(i);
        mLog.Printf(LT_MESSAGE, L"    %d: %s", i, deviceName);
    }

    int soundDevice = -1;
    if (mSettings.mSound.mDevice.EqualW(L"Default"))
    {
        #if DISABLE_VR==0
        if (mHMD)
        {
            void* deviceGUID = mHMD->GetSoundOutputGUID();
            if (deviceGUID == nullptr)
            {
                soundDevice = -1;
            }
            else
            {
                soundDevice = mSoundEngineBackend->GetDeviceFromGUID(deviceGUID);
                if (soundDevice == -1)
                {
                    mLog.Printf(LT_WARNING, L"Headset headphones are off. Switching to default sound device");
                    soundDevice = -1;
                }
            }
        }
        else
        #endif
        {
            soundDevice = -1;
        }
    }
    else
    {
        soundDevice = mSoundEngineBackend->GetDeviceFromName(mSettings.mSound.mDevice.GetS());
        if (soundDevice == -1)
        {
            mLog.Printf(LT_ERROR, L"Couldn't find specified sound device");
            mSettings.End();
            return false;
        }
    }

    const wchar_t *deviceName = (soundDevice == -1) ? L"Default" : mSoundEngineBackend->GetDeviceName(soundDevice);
    mLog.Printf(LT_MESSAGE, L"Sound device selected: \"%s\", requested \"%s\"", deviceName, mSettings.mSound.mDevice.GetS());

    piSoundEngineBackend::Configuration config;

    if (!mSoundEngineBackend->Init(piWindow_getHandle(mWindow), soundDevice, &config)) // TODO: copy max sounds setting from app
    {
        mSettings.End();
        return false;
    }



    const int vpmult = (mStereoMode == ImmPlayer::StereoMode::Preferred) ? 2 : 1;

    if (mRenderer->GetAPI() != piRenderer::API::DX)
    {
        const piRenderer::TextureInfo infocm = { piRenderer::TextureType::T2D, piRenderer::Format::C3_11_11_10_FLOAT, mRenderSize.x * vpmult * mSuperSample, mRenderSize.y * mSuperSample, 1, AA };
        const piRenderer::TextureInfo infozm = { piRenderer::TextureType::T2D, piRenderer::Format::DS_24_8_UINT, mRenderSize.x * vpmult * mSuperSample, mRenderSize.y * mSuperSample, 1, AA };
        mColorTextureM = mRenderer->CreateTexture(0, &infocm, false, piRenderer::TextureFilter::NONE, piRenderer::TextureWrap::CLAMP, 1.0f, 0); if (!mColorTextureM) return false;
        mDepthTextureM = mRenderer->CreateTexture(0, &infozm, false, piRenderer::TextureFilter::NONE, piRenderer::TextureWrap::CLAMP, 1.0f, 0); if (!mDepthTextureM) return false;
    }
    else
    {
        const piRenderer::TextureInfo infocm = { piRenderer::TextureType::T2D, piRenderer::Format::C3_11_11_10_FLOAT, mRenderSize.x * vpmult * mSuperSample, mRenderSize.y * mSuperSample, 1, AA };
        const piRenderer::TextureInfo infozm = { piRenderer::TextureType::T2D, piRenderer::Format::DS_24_8_UINT,            mRenderSize.x * vpmult * mSuperSample, mRenderSize.y * mSuperSample, 1, AA };
        //const piRenderer::TextureInfo2 infozm = { piRenderer::TextureType::T2D, piRenderer::Format::D1_32_FLOAT,            mRenderSize.x * vpmult * mSuperSample, mRenderSize.y * mSuperSample, 1, AA };
        mColorTextureM = mRenderer->CreateTexture2(0, &infocm, false, piRenderer::TextureFilter::NONE, piRenderer::TextureWrap::CLAMP, 1.0f, 0, 1 + 2); if (!mColorTextureM) return false;
        mDepthTextureM = mRenderer->CreateTexture2(0, &infozm, false, piRenderer::TextureFilter::NONE, piRenderer::TextureWrap::CLAMP, 1.0f, 0, 2); if (!mDepthTextureM) return false;
    }
    if (!mColorTextureM || !mDepthTextureM)
    {
        mSettings.End();
        return false;
    }
    mRenderTargetM = mRenderer->CreateRenderTarget(mColorTextureM, 0, 0, 0, mDepthTextureM); if (!mRenderTargetM) return false;


    if (!mResolve.Init(mRenderer, mSuperSample))
    {
        mSettings.End();
        return false;
    }

    piSoundEngine* soundEngine = mSoundEngineBackend->GetEngine();

    if (!mViewer.Init(nullptr, mRenderer, soundEngine, &mLog, &mTimer, mStereoMode, &mSettings))
    {
        mSettings.End();
        return false;
    }

    // enter render loop

    double to = mTimer.GetTime();
    double renderFpsTo = 0.0;
    int renderFrame = 0;
    float renderFps = 0.0;
    int totalFrames = 0;
    int done = 0;
    bool doSave = true;
    double oldTime;
    bool enabled = true;

    oldTime = to;


#if defined(WINDOWS)
    static const uint32_t kRenderBudgetMicroseconds = 9000;
#elif defined(ANDROID)
    static const uint32_t kRenderBudgetMicroseconds = 5000;
#endif


    mLog.Printf(LT_MESSAGE, L"X = next,  Z = prev,  C = restart,   v = replay,   P = pause/resume");
    int frameid = 0;
    bool isFirstFrame = true;

    while (!done)
    {
        frameid++;
        const double time = mTimer.GetTime() - to;
        const float dtime = float(time - oldTime);
        oldTime = time;

        float mQuitFade = 1.0f;

        // events
        piWindowEvents_Erase(mWindow);
        piWindowMgr_MessageLoop(mWinMgr);
        done |= piWindow_getExitReq(mWindow);
        piWindowEvents* evt = piWindow_getEvents(mWindow);
        piWindowEvents_GetMouse_D(&evt->mouse);

        #if DISABLE_VR==0
        if (mStereoMode != ImmPlayer::StereoMode::None)
        {
            int tid[2];
            bool needMipMapping;
            mHMD->BeginFrame(tid + 0, tid + 1, &needMipMapping);

            const trans3d vr_to_head = fromMatrix(f2d(mat4x4(mHMD->mInfo.mHead.mCamera)));

            mViewer.GlobalWork(evt, mStereoMode != ImmPlayer::StereoMode::None, vr_to_head, mHMD->mInfo.mController, &mHMD->mInfo.mRemote, &mLog, dtime, mWindowSize, enabled ? 1 : 0, kRenderBudgetMicroseconds, isFirstFrame);


            mViewer.GlobalRender(vr_to_head, vec4(mHMD->mInfo.mHead.mProjection));

            if (mStereoMode == ImmPlayer::StereoMode::Fallback)
            {
                const int vpS[4] = { 0, 0, mRenderSize.x, mRenderSize.y };
                const int vpM[4] = { 0, 0, mRenderSize.x * mSuperSample, mRenderSize.y * mSuperSample };
                for (int i = 0; i < 2; i++)
                {
                    mRenderer->SetRenderTarget(mRenderTargetM);
                    mRenderer->SetViewport(0, vpM);
                    mRenderer->SetWriteMask(true, false, false, false, true);
                    const mat4x4d headToEye = f2d(mat4x4(mHMD->mInfo.mEye[i].mCamera)) * invert(f2d(mat4x4(mHMD->mInfo.mHead.mCamera)));
                    mViewer.RenderStereoMultiPass(mRenderSize*mSuperSample, i, headToEye, vec4(mHMD->mInfo.mEye[i].mProjection), vr_to_head);
                    // resolve multisampling and postpro
                    mResolve.Do(mRenderer, mTextureChain[i].mRenderTarget[tid[i]], vpS, 0, mQuitFade, mColorTextureM);
                }
            }
            else
            {
                mRenderer->SetRenderTarget(mRenderTargetM);
                #if 0
                const int vpL[4] = {                            0, 0, mRenderSize.x * mSuperSample, mRenderSize.y * mSuperSample };
                const int vpR[4] = { mRenderSize.x * mSuperSample, 0, mRenderSize.x * mSuperSample, mRenderSize.y * mSuperSample };
                mRenderer->SetViewport(0, vpL);
                mRenderer->SetViewport(1, vpR);
                #else
                const float data[12] = {
                    0.0f,                              0.0f, float(mRenderSize.x*mSuperSample), float(mRenderSize.y*mSuperSample), 0.0f, 0.0f,
                    float(mRenderSize.x*mSuperSample), 0.0f, float(mRenderSize.x*mSuperSample), float(mRenderSize.y*mSuperSample), 0.0f, 0.0f };
                mRenderer->SetViewports(2, data);
                #endif
                const mat4x4d headToLEye = f2d(mat4x4(mHMD->mInfo.mEye[0].mCamera)) * invert(f2d(mat4x4(mHMD->mInfo.mHead.mCamera)));
                const mat4x4d headToREye = f2d(mat4x4(mHMD->mInfo.mEye[1].mCamera)) * invert(f2d(mat4x4(mHMD->mInfo.mHead.mCamera)));

                mRenderer->SetWriteMask(true, false, false, false, true);
                mViewer.RenderStereoSinglePass(mRenderSize*mSuperSample, vr_to_head, headToLEye, vec4(mHMD->mInfo.mEye[0].mProjection), headToREye, vec4(mHMD->mInfo.mEye[1].mProjection), mHMD);
                // resolve multisampling and postpro
                for (int i = 0; i < 2; i++)
                {
                    const int unXOffset = i * mRenderSize.x;
                    mResolve.Do(mRenderer, mTextureChain[i].mRenderTarget[tid[i]], mHMD->mInfo.mEye[i].mVP, unXOffset, mQuitFade, mColorTextureM);
                }
            }

            // compute mipmaps before distortion occurs
            mRenderer->ComputeMipmaps(mTextureChain[0].mRenderTexture[tid[0]]);
            if (needMipMapping)
            {
                mRenderer->ComputeMipmaps(mTextureChain[1].mRenderTexture[tid[1]]);
            }

            // mirror
            const int wvp[4] = { 0, 0, mWindowSize.x, mWindowSize.y };
            mRenderer->SetRenderTarget(nullptr);
            mRenderer->SetViewport(0, wvp);
            mRenderer->SetWriteMask(true, false, false, false, false);
            mRenderer->SetState(piSTATE_CULL_FACE, false);
            mRenderer->SetState(piSTATE_DEPTH_TEST, false);
            // mRenderer->AttachSamplers(1, mMirrorSampler );
            mRenderer->AttachTextures(1, mTextureChain[0].mRenderTexture[tid[0]]);
            // mRenderer->AttachTextures(1, mMirrorRenderTexture);
            mRenderer->AttachShader(mMirrorShader);
            const float data[2] = { float(mWindowSize.x), float(mWindowSize.y) };
            mRenderer->SetShaderConstant2F(1, data, 1);
            //mRenderer->SetShaderConstantSampler(0, 0);
            mRenderer->DrawUnitQuad_XY(1);
            mRenderer->DettachTextures();
            mRenderer->DettachSamplers();
            mRenderer->DettachShader();

            mHMD->EndFrame();

            mRenderer->SwapBuffers();
        }
        else
        #endif
        {
            const int vpM[4] = { 0, 0, mRenderSize.x * mSuperSample, mRenderSize.y * mSuperSample };
            const trans3d vr_to_head = trans3d::translate(vec3d(0.0, -1.6, 0.0));

            mRenderer->SetRenderTarget(mRenderTargetM);
            mRenderer->SetViewport(0, vpM);
			mRenderer->SetWriteMask(true, false, false, false, true);
            #if DISABLE_VR==0
            mViewer.GlobalWork(evt, mStereoMode != ImmPlayer::StereoMode::None, vr_to_head, mHMD->mInfo.mController, &mHMD->mInfo.mRemote, &mLog, dtime, mWindowSize, enabled ? 1 : 0, kRenderBudgetMicroseconds, isFirstFrame);
            #else
            mViewer.GlobalWork(evt, mStereoMode != ImmPlayer::StereoMode::None, vr_to_head, nullptr, nullptr, &mLog, dtime, mWindowSize, enabled ? 1 : 0, kRenderBudgetMicroseconds, isFirstFrame);
            #endif

            // render
            mViewer.GlobalRender(vr_to_head, vec4(0.0f));
            // render
            mViewer.RenderMono(mRenderSize*mSuperSample, vr_to_head, 0);
            // resolve multisampling and postpro
            mResolve.Do(mRenderer, nullptr, vpM, 0, mQuitFade, mColorTextureM);

            mRenderer->SwapBuffers();
        }

        mSoundEngineBackend->Tick();

        totalFrames++;

        // update fps counter
        renderFrame++;
        const double dt = time - renderFpsTo;
        if (dt > 1.0)
        {
            renderFps = (float)renderFrame / (float)dt;
            renderFrame = 0;
            renderFpsTo = time;
        }
        if ((totalFrames & 63) == 0)
        {
            wchar_t str[64];
            piwsprintf(str, 63, L"%.1f fps :: %.2f", renderFps, time);
            piWindow_setText(mWindow, str);
        }
    }

    mViewer.Deinit();

    mSoundEngineBackend->Deinit();

    piDestroySoundEngineBackend(mSoundEngineBackend);

    mResolve.DeInit(mRenderer);

    mRenderer->DestroyRenderTarget(mRenderTargetM);
    mRenderer->DestroyTexture(mDepthTextureM);
    mRenderer->DestroyTexture(mColorTextureM);
    #if DISABLE_VR==0
    if (mStereoMode != ImmPlayer::StereoMode::None)
    {
        mRenderer->DestroyShader(mMirrorShader);
        for (int j = 0; j < 2; j++)
        {
            for (int i = 0; i < mTextureChain[j].mRenderNumTextures; i++)
            {
                mRenderer->DestroyTexture(mTextureChain[j].mRenderTexture[i]);
                mRenderer->DestroyRenderTarget(mTextureChain[j].mRenderTarget[i]);
            }
        }
    }

    if (mStereoMode != ImmPlayer::StereoMode::None)
    {
        piVRHMD::Destroy(mHMD);
    }
    #endif

    mRenderer->Deinitialize();
    mRenderer->Report();

    delete mRenderReporter;
    delete mRenderer;
    piWindow_end(mWindow);
    piWindowMgr_End(mWinMgr);

    mSettings.End();

    mLog.End();

    return 1;
}
