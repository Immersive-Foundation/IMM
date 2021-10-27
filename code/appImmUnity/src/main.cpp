// ------------------------
// Host API
// ------------------------
// bool Init( Options )                                // once per process and session
// void End()                                           // once per process and session
// void GlobalWork(objectToWorldMatrix)                 // once per frame, global for all cameras
// void SetMatrices(cameraID, stereoType, ... )         // once per frame and per camera
// PlayerInfo GetPlayerInfo()                           // live data, query every frame
//
//
// Options = { ColorSpace, Antialiasing, RequestViewpointDisplay }
//
// PlayerInfo =
// {
//	  Array of Viewpoints
//	  {
//        Name
//        Location // position, scale and orientation
//        Extent   // box or sphere volume of allowed viewer locations
//        Allowed  // locomotion restrictions, bitmask of { TranslateHorizontal | TranslateVertical | RotateHoritonal | RotateVertical | Scale }
//    }
//    Lighting
//    {
//        BackgroundColor
//        Array of Colorizers
//        {
//            Type       // { Sphere, Point, Gradient, ... }
//            Paramaters = { ... }
//        }
//    }
//    Array of Grablables
//    {
//    }
// }
//
// ------------------------
// Document API
// ------------------------
// id   Load(char *fileName)
// id   LoadStreaming(char *url)
// void Unload(id)
// DocumentInfo  GetDocumentInfo(id)
//
// DocumentInfo
// {
// 	 DocumentType // { Still, Animated, Comic }
// 	 IsGrabbable
// 	 HasSound
// }
//
//
// ------------------------
// Playback API
// ------------------------
// void   Next(id)           // jump to next chapter
// void   Prev(id)           // jump to prev chapter
// void   Pause(id)          // pause
// void   Resume(id)         // resume
// void   Restart(id)        // restart the whole comic
// void   Replay(id)         // restart the current chapter
// bool   HasNext(id)        // is it possible to call Next
// bool   HasPrev(id)        // is it possible to call Prev
// void   Show(id)           // show
// void   Hide(id)           // hide
// void   SetVolume(volvume) // sound volume
// time   GetSynchTime()     // to synch across clients
// void   SetSynchTime(time) // to synch across clients
//
// DocumentState GetDocumentState(id);
//
// DocumentState
// {
//	   LoadingState  // { Unloaded, Loading, Loaded, Unloading, Failed }
//	   PlaybackState // { Playing, Paused, PausedAndHidden, Waiting, Finished }
// }


#define VERBOSE 0


#include "libImmCore/src/libBasics/piStr.h"
#include "libImmPlayer/src/player.h"
#include "libImmImporter/src/document/layerSpawnArea.h"
#include "IUnityGraphics.h"
#if !defined(__ANDROID__) && !defined(ANDROID)
#include "IUnityGraphicsD3D11.h"
#include "IUnityGraphicsD3D12.h"
#endif
using namespace ImmCore;
using namespace ImmImporter;
using namespace ImmPlayer;

#if defined(__ANDROID__) || defined(ANDROID)
#define _stdcall
#include <android/log.h>
#endif

// ----------------------------------------------------------------------------------------------------------------------------------------------------
class MainRenderReporter : public piRenderer::piReporter
{
private:
	piLog* mLog;

public:
	MainRenderReporter(piLog* log) : piRenderer::piReporter() { mLog = log; }
	virtual ~MainRenderReporter() {}
	void Info(const char* str)
	{
		piString wstr;
		wstr.InitCopyS(str);
		mLog->Printf(LT_MESSAGE, L"%s", wstr.GetS());
		wstr.End();
	}
	void Error(const char* str, int level)
	{
		piString wstr;
		wstr.InitCopyS(str);
		mLog->Printf(LT_ERROR, L"%s", wstr.GetS());
		wstr.End();
	}
	void Begin(uint64_t memCurrent, uint64_t memPeak, int texCurrent, int texPeak)
	{
		mLog->Printf(LT_MESSAGE, L"---- Renderer Report ---- ");
		mLog->Printf(LT_MESSAGE, L"Peak: %d MB in %d textures", memPeak >> 20, texPeak);
		mLog->Printf(LT_MESSAGE, L"Curr: %d MB in %d textures", memCurrent >> 20, texCurrent);
	}
	void End(void)
	{
		mLog->Printf(LT_MESSAGE, L"---- Renderer Report ---- ");
	}
	void Texture(const wchar_t* key, uint64_t kb, piRenderer::Format format, bool compressed, int xres, int yres, int zres)
	{
		mLog->Printf(LT_MESSAGE, L"* Texture: %5d kb, %4d x %4d x %4d %2d (%s)", (int)kb, xres, yres, zres, format,
			(key == nullptr) ? L"null" : key);
	}
};

// We create ONE of these per Unity process
struct ImmUnityPlugin
{
	//----------------------------
	// Unity provided classes
	//----------------------------
	struct
	{
		IUnityInterfaces * mUnityInterfaces = nullptr;
		IUnityGraphics   * mGraphics = nullptr;
		void             * mDevice;
	}UnityAPI;

	//----------------------------
	// Provided info from Unity
	//----------------------------
	struct
	{
		struct
		{
			int          mStereoType;    // 0=mono, 1=two pass, 2=single pass
			int          mCurrentEye;
			mat4x4       mWorld2Head;
			mat4x4       mWorld2LEye;
			mat4x4       mWorld2REye;
			mat4x4       mHeadProjection;
			mat4x4       mLEyeProjection;
			mat4x4       mREyeProjection;
		}mCamera[256];
	}FromUnity;

	//----------------------------
	// IMM classes
	//----------------------------
	struct
	{
		piRenderer * mRenderer;
		MainRenderReporter *mRenderReporter;
		piLog        mLog;
        piSoundEngineBackend* mSoundBackend;
		piTimer      mTimer;
		Player       mPlayer; // actual player.
	}IMM;
};

// ----------------------------------------------------------------------------------------------------------------------------------------------------

// this is the only global data structure, live cycle is plugin load/unload
static ImmUnityPlugin gImmUnityPlugin;

static void UNITY_INTERFACE_API iOnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	// Create graphics API implementation upon initialization
	if (eventType == kUnityGfxDeviceEventInitialize)
	{
		UnityGfxRenderer apiType = gImmUnityPlugin.UnityAPI.mGraphics->GetRenderer();

#if !defined(__ANDROID__) && !defined(ANDROID)
		if (apiType == kUnityGfxRendererD3D11)
		{
			IUnityGraphicsD3D11* ud3d = gImmUnityPlugin.UnityAPI.mUnityInterfaces->Get<IUnityGraphicsD3D11>();
			gImmUnityPlugin.UnityAPI.mDevice = ud3d->GetDevice();
		}
		else if (apiType == kUnityGfxRendererD3D12)
		{
			IUnityGraphicsD3D12v2* ud3d = gImmUnityPlugin.UnityAPI.mUnityInterfaces->Get<IUnityGraphicsD3D12v2>();
			gImmUnityPlugin.UnityAPI.mDevice = ud3d->GetDevice();
		}
		else if(apiType == kUnityGfxRendererOpenGLCore || apiType == kUnityGfxRendererOpenGLES20 || apiType == kUnityGfxRendererOpenGLES30)
		{
			gImmUnityPlugin.UnityAPI.mDevice = nullptr;
		}
#else
		if (apiType == kUnityGfxRendererOpenGLES30)
		{
			gImmUnityPlugin.UnityAPI.mDevice = nullptr;
			gImmUnityPlugin.IMM.mLog.Printf(LT_MESSAGE, L"kUnityGfxDeviceEventInitialize using OpenGL ES 3.0 device");
		}
#endif
	}
	else if (eventType == kUnityGfxDeviceEventShutdown)
	{
	}
}

static void UNITY_INTERFACE_API iOnRenderEvent(int event_id)
{
	int numVp = 1;
	float oldVp[6];
	gImmUnityPlugin.IMM.mRenderer->GetViewports(&numVp, oldVp);
	if (numVp < 1) return;

	//const int eventType = (event_id >> 0) & 0xff;
	const int cameraID  = (event_id >> 8) & 0xff;
	const int stereoType = gImmUnityPlugin.FromUnity.mCamera[cameraID].mStereoType;
	const ivec2 res = ivec2(int(oldVp[2]), int(oldVp[3]));

	if (stereoType == 0) // mono
	{

		gImmUnityPlugin.IMM.mPlayer.GlobalRender(fromMatrix(f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2Head)),
            fromMatrix(f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2Head)),
            gImmUnityPlugin.FromUnity.mCamera[cameraID].mHeadProjection, StereoMode::None);
		gImmUnityPlugin.IMM.mPlayer.RenderMono(res,0);
	}
	else if (stereoType == 1) // two pass stereo
	{

		const int eyeID = gImmUnityPlugin.FromUnity.mCamera[cameraID].mCurrentEye & 1;
		gImmUnityPlugin.FromUnity.mCamera[cameraID].mCurrentEye++;

        gImmUnityPlugin.IMM.mPlayer.GlobalRender(fromMatrix(f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2Head)),
            fromMatrix(f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2Head)),
            gImmUnityPlugin.FromUnity.mCamera[cameraID].mHeadProjection, StereoMode::Fallback);

		if (eyeID == 0) // left eye
		{
			const mat4x4d head_to_lEye = f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2LEye) *
                invert(f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2Head));
			gImmUnityPlugin.IMM.mPlayer.RenderStereoMultiPass(res, 0,
				head_to_lEye,
				gImmUnityPlugin.FromUnity.mCamera[cameraID].mLEyeProjection);
		}
		else // right eye
		{
			const mat4x4d head_to_rEye = f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2REye) *
                invert(f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2Head));

			gImmUnityPlugin.IMM.mPlayer.RenderStereoMultiPass(res, 1,
				head_to_rEye,
				gImmUnityPlugin.FromUnity.mCamera[cameraID].mREyeProjection);
		}
	}
	else if (stereoType == 2) // single pass stereo
	{
        //gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"Single pass rendering...");

		// This all block here is a hack to fix the funny behavior of Unity in SinglePass stereo.
		//
		// Even if the render target is 2x the size to accommodate the left and right eyes, Unity
		// will set the viewport to only cover the left eye. From an email response from Scorr Bassett:
		//
		//   "That behavior is correct.The single - pass mode that you are using does the following :
		//
		//    1. Set viewport to the left
		// 	  2. Issue Draw
		// 	  3. Set viewport to the right(internal)
		// 	  4. Issue Draw(internal)
		//
		//    Therefore the viewport size will never be the full size.Since you are using a plug - in
		//    and doing your own thing we don't handle that. It's up to you to change the viewport and
		//    restore it afterwards."
		//
		// So there we go. In my case, I make the Imm Player left and right eyes in a single GPU geometry pass,
		// so I have to set the viewport to the corret 2X size here, then restore it.
		const float newVp[6] = { oldVp[0], oldVp[1], oldVp[2]*2.0f, oldVp[3], oldVp[4], oldVp[5] };
		gImmUnityPlugin.IMM.mRenderer->SetViewports(1, newVp);

		//const mat4x4d flz = mat4x4d(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0);

		gImmUnityPlugin.IMM.mPlayer.GlobalRender(fromMatrix(f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2Head)), fromMatrix(f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2Head)), gImmUnityPlugin.FromUnity.mCamera[cameraID].mHeadProjection, StereoMode::Preferred);
		const mat4x4d head_to_lEye = f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2LEye) * invert(f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2Head));
		const mat4x4d head_to_rEye = f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2REye) * invert(f2d(gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2Head));
		gImmUnityPlugin.IMM.mPlayer.RenderStereoSinglePass( res, head_to_lEye, gImmUnityPlugin.FromUnity.mCamera[cameraID].mLEyeProjection,
 			                                                          head_to_rEye, gImmUnityPlugin.FromUnity.mCamera[cameraID].mREyeProjection);
		gImmUnityPlugin.IMM.mRenderer->SetViewports(1, oldVp);
	}
    gImmUnityPlugin.IMM.mSoundBackend->Tick();

}

// ----------------------------------------------------------------------------------------------------------------------------------------------------
// PLUGIN INTERFACE
// ----------------------------------------------------------------------------------------------------------------------------------------------------


extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
	gImmUnityPlugin.UnityAPI.mUnityInterfaces = unityInterfaces;
	gImmUnityPlugin.UnityAPI.mGraphics = gImmUnityPlugin.UnityAPI.mUnityInterfaces->Get<IUnityGraphics>();
	gImmUnityPlugin.UnityAPI.mGraphics->RegisterDeviceEventCallback(iOnGraphicsDeviceEvent);

	// Run OnGraphicsDeviceEvent(initialize) manually on plugin load
	iOnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	gImmUnityPlugin.UnityAPI.mGraphics->UnregisterDeviceEventCallback(iOnGraphicsDeviceEvent);
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
	return iOnRenderEvent;
}

extern "C" void UNITY_INTERFACE_EXPORT Debug()
{
	if(gImmUnityPlugin.IMM.mRenderer == nullptr)
        gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"Renderer has not initialized");
    else
        gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"Renderer has initialized");

}

static mat4x4 iUnityToPilibs(const float *m)
{
	return mat4x4(m[0], m[4], m[ 8], m[12],
		          m[1], m[5], m[ 9], m[13],
		          m[2], m[6], m[10], m[14],
		          m[3], m[7], m[11], m[15]);
}


extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GlobalWork(int enabled)
{
    gImmUnityPlugin.IMM.mPlayer.GlobalWork(enabled == 1, 9000);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetMatrices( int cameraID, int stereoType,
	                                                                    float *world2head, float *prjHead,
	                                                                    float *world2leye, float *prjLeft,
	                                                                    float *world2reye, float *prjRight )
{
	if (cameraID > 255)return;

	gImmUnityPlugin.FromUnity.mCamera[cameraID].mStereoType = stereoType;
	gImmUnityPlugin.FromUnity.mCamera[cameraID].mCurrentEye = 0;

	if(world2head !=nullptr ) gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2Head = iUnityToPilibs(world2head);
	if(world2leye != nullptr) gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2LEye = iUnityToPilibs(world2leye);
	if(world2reye != nullptr) gImmUnityPlugin.FromUnity.mCamera[cameraID].mWorld2REye = iUnityToPilibs(world2reye);
	if(prjHead    !=nullptr ) gImmUnityPlugin.FromUnity.mCamera[cameraID].mHeadProjection = iUnityToPilibs(prjHead);
	if(prjLeft    !=nullptr ) gImmUnityPlugin.FromUnity.mCamera[cameraID].mLEyeProjection = iUnityToPilibs(prjLeft);
	if(prjRight   !=nullptr ) gImmUnityPlugin.FromUnity.mCamera[cameraID].mREyeProjection = iUnityToPilibs(prjRight);
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API Init( int colorSpace, // 0=linear 1=gamma
	                                        int antialiasing, // 8
											char *logFileName,
											char *tmpFolferName)
{
	const wchar_t *wstrLogFileName = (logFileName == nullptr) ? L"imm_player_log.txt" : pistr2ws(logFileName);

#if defined(__ANDROID__) || defined(ANDROID)
#else // !ANDROID
	#ifdef _DEBUG
	if (!gImmUnityPlugin.IMM.mLog.Init(wstrLogFileName, PILOG_TXT + PILOG_CNS))
	#else
	if (!gImmUnityPlugin.IMM.mLog.Init(wstrLogFileName, PILOG_TXT))
	#endif
		return -1;
	gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, colorSpace == 0 ? L"Linear": L"Gamma");
    gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"Antialiasing: %d", antialiasing);
    gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"Log File: %s", wstrLogFileName);
#endif // ANDROID

	if (!gImmUnityPlugin.IMM.mTimer.Init())
		return -1;

    // SOUND ENGINE
    gImmUnityPlugin.IMM.mSoundBackend = piCreateSoundEngineBackend(piSoundEngineBackend::API::DirectSoundOVR,&gImmUnityPlugin.IMM.mLog);

    if(!gImmUnityPlugin.IMM.mSoundBackend)
        gImmUnityPlugin.IMM.mLog.Printf(LT_ERROR, L"Failed to create SoundBackend.");
    else
		gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"SoundBackend created successfully.");

    // start with the default sound device
    int deviceID = -1;
    // but try to find a "Rift" sound device
    {
        const int num = gImmUnityPlugin.IMM.mSoundBackend->GetNumDevices();
        for (int i = 0; i < num; i++)
        {
            const wchar_t * deviceName = gImmUnityPlugin.IMM.mSoundBackend->GetDeviceName(i);
            if (piwstrcontains(deviceName, L"Rift"))
            {
                deviceID = i;
                break;
            }

        }
    }
    piSoundEngineBackend::Configuration config;

    if (!gImmUnityPlugin.IMM.mSoundBackend->Init(nullptr, deviceID, &config))
    {
		gImmUnityPlugin.IMM.mLog.Printf(LT_ERROR, L"Failed to initialize SoundBackend.");
        return -1;
    }
	gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"SoundBackend initialized successfully.");

    // RENDERER
	const char* apiName[] = {"GL", "DX", "GLES"};

#if defined(__ANDROID__) || defined(ANDROID)
	const piRenderer::API api = piRenderer::API::GLES;
	gImmUnityPlugin.IMM.mRenderReporter = nullptr;
#else
	const piRenderer::API api = (gImmUnityPlugin.UnityAPI.mDevice == nullptr) ? piRenderer::API::GL : piRenderer::API::DX;
	gImmUnityPlugin.IMM.mRenderReporter = new MainRenderReporter(&gImmUnityPlugin.IMM.mLog);
#endif
	gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"API: %s", pistr2ws(apiName[static_cast<int>(api)]));

	gImmUnityPlugin.IMM.mRenderer = piRenderer::Create(api);
    if (!gImmUnityPlugin.IMM.mRenderer)
    {
        gImmUnityPlugin.IMM.mLog.Printf(LT_ERROR, L"Failed to create Renderer.");
		return -1;
    }
    gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"Renderer created successfully");

#if defined(__ANDROID__) || defined(ANDROID)
	if (!gImmUnityPlugin.IMM.mRenderer->Initialize(0, nullptr, 1, false, false, gImmUnityPlugin.IMM.mRenderReporter, false, nullptr))
#else
    if (!gImmUnityPlugin.IMM.mRenderer->Initialize(0, nullptr, 0, true, false, gImmUnityPlugin.IMM.mRenderReporter, false, gImmUnityPlugin.UnityAPI.mDevice))
#endif
    {
        gImmUnityPlugin.IMM.mLog.Printf(LT_ERROR, L"Failed to initialize Renderer.");
        return -1;
    }
	gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"Renderer initialized successfully.");

	// PLAYER
    Player::Configuration conf;
#if defined(__ANDROID__) || defined(ANDROID)
    conf.colorSpace = Drawing::ColorSpace::Gamma;
    conf.depthBuffer = DepthBuffer::Linear01;
    conf.clipDepth = ClipSpaceDepth::FromNegativeOneToOne;
    conf.projectionMatrix = ClipSpaceDepth::FromNegativeOneToOne;
    conf.frontIsCCW = true;
    conf.paintRenderingTechnique = Drawing::PaintRenderingTechnique::Static;
#else
    conf.colorSpace = static_cast<Drawing::ColorSpace>(colorSpace);
    gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"ColorSpace: %s", conf.colorSpace == Drawing::ColorSpace::Gamma ? L"Gamma" : L"Linear" );

    conf.multisamplingLevel = antialiasing;
    // NOTE. THESE SHOULD BE PASSED IN THE INIT OF THE DLL. But for now we hardcode it her since our only clients are Unity in DX or GLES modes
    conf.depthBuffer      = (api == piRenderer::API::DX) ? DepthBuffer::Linear10         : DepthBuffer::Linear01;
    conf.clipDepth        = (api == piRenderer::API::DX) ? ClipSpaceDepth::FromZeroToOne : ClipSpaceDepth::FromNegativeOneToOne;
    conf.projectionMatrix = (api == piRenderer::API::DX) ? ClipSpaceDepth::FromZeroToOne : ClipSpaceDepth::FromNegativeOneToOne;
    conf.frontIsCCW       = (api == piRenderer::API::DX) ? false                         : true;
    conf.paintRenderingTechnique = Drawing::PaintRenderingTechnique::Static;
    gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"Rending in Static mode");

#endif
    if (!gImmUnityPlugin.IMM.mPlayer.Init(gImmUnityPlugin.IMM.mRenderer, gImmUnityPlugin.IMM.mSoundBackend->GetEngine(), &gImmUnityPlugin.IMM.mLog, &gImmUnityPlugin.IMM.mTimer, &conf))
	{
        gImmUnityPlugin.IMM.mLog.Printf(LT_ERROR, L"Failed to initialize ImmPlayer.");
		gImmUnityPlugin.IMM.mSoundBackend->Deinit();
		gImmUnityPlugin.IMM.mRenderer->Deinitialize();
		gImmUnityPlugin.IMM.mLog.End();
		gImmUnityPlugin.IMM.mTimer.End();
		return -2;
	}
    gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"IMMl Player initialized successfully.");

	return 0;
}

extern "C" void UNITY_INTERFACE_EXPORT End(void)
{
    gImmUnityPlugin.IMM.mPlayer.UnloadAllSync();
    //gImmUnityPlugin.IMM.mPlayer.GlobalWork(0,9000);
    gImmUnityPlugin.IMM.mPlayer.Deinit();
    gImmUnityPlugin.IMM.mRenderer->Deinitialize();

    gImmUnityPlugin.IMM.mSoundBackend->Deinit();
    piDestroySoundEngineBackend(gImmUnityPlugin.IMM.mSoundBackend);
    gImmUnityPlugin.IMM.mTimer.End();
	gImmUnityPlugin.IMM.mLog.End();
}

extern "C" int UNITY_INTERFACE_EXPORT LoadFromFile(char *fileName)
{
    gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"loading from file: %s", pistr2ws(fileName));

    return gImmUnityPlugin.IMM.mPlayer.Load(pistr2ws(fileName));
}

extern "C" int UNITY_INTERFACE_EXPORT LoadFromMemory(char *fileName, int size, void* data) // ToDo: need to pass data from managed code to native code.
{
    if (!data || size == 0)
    {
        gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"loading from memory...\nfile name is %s\nsize is %d", pistr2ws(fileName), size);
        gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"Data is empty");
        return -1;

    }
    gImmUnityPlugin.IMM.mLog.Printf(LT_DEBUG, L"loading from memory...\nfile name is %s\nsize is %d", pistr2ws(fileName), size);

    piTArray<uint8_t> imm;
    imm.Init(0, false);
    imm.Set((uint8_t*)(data), (uint64_t)(size));
    return gImmUnityPlugin.IMM.mPlayer.Load(&imm, pistr2ws(fileName));

}

extern "C" void UNITY_INTERFACE_EXPORT Unload(int id)
{
	gImmUnityPlugin.IMM.mPlayer.Unload(id);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetDocumentToWorld(int id, float *doc2world)
{
	gImmUnityPlugin.IMM.mPlayer.SetDocumentToWorld(id, fromMatrix(f2d(iUnityToPilibs(doc2world)) * mat4x4d::flipZ()));
}

extern "C" void UNITY_INTERFACE_EXPORT Pause(int id)
{
	gImmUnityPlugin.IMM.mPlayer.Pause(id );
}

extern "C" void UNITY_INTERFACE_EXPORT Resume(int id)
{
	gImmUnityPlugin.IMM.mPlayer.Resume(id);
}

extern "C" void UNITY_INTERFACE_EXPORT Hide(int id)
{
	gImmUnityPlugin.IMM.mPlayer.Hide(id);
}

extern "C" void UNITY_INTERFACE_EXPORT Show(int id)
{
	gImmUnityPlugin.IMM.mPlayer.Show(id);
}

extern "C" void UNITY_INTERFACE_EXPORT Continue(int id)
{
	gImmUnityPlugin.IMM.mPlayer.Continue(id);
}

extern "C" void UNITY_INTERFACE_EXPORT SkipForward(int id)
{
    gImmUnityPlugin.IMM.mPlayer.SkipForward(id);
}

extern "C" void UNITY_INTERFACE_EXPORT SkipBack(int id)
{
	gImmUnityPlugin.IMM.mPlayer.SkipBack(id);
}

extern "C" void UNITY_INTERFACE_EXPORT Restart(int id)
{
	gImmUnityPlugin.IMM.mPlayer.Restart(id);
}


extern "C" int UNITY_INTERFACE_EXPORT GetChapterCount(int id)
{
    return gImmUnityPlugin.IMM.mPlayer.GetChapterCount(id);
}

extern "C" int UNITY_INTERFACE_EXPORT GetCurrentChapter(int id)
{
    return gImmUnityPlugin.IMM.mPlayer.GetCurrentChapter(id);
}

extern "C" void UNITY_INTERFACE_EXPORT SetTime(int id, int64_t timeSinceStart, int64_t timeSinceStop)
{
	gImmUnityPlugin.IMM.mPlayer.SetTime(id, piTick(timeSinceStart), piTick(timeSinceStop));
}

extern "C" void UNITY_INTERFACE_EXPORT GetTime(int id, int64_t * timeSinceStart, int64_t * timeSinceStop)
{
	gImmUnityPlugin.IMM.mPlayer.GetTime(id, (piTick*)timeSinceStart, (piTick*)timeSinceStop);
}

extern "C" int64_t UNITY_INTERFACE_EXPORT GetPlayTime(int id)
{
    piTick startTime;
    piTick stopTime;

    gImmUnityPlugin.IMM.mPlayer.GetTime(id, &startTime, &stopTime);

    return piTick::CastInt(startTime);
}

extern "C" void UNITY_INTERFACE_EXPORT GetPlayerInfo(Player::PlayerInfo & info)
{
	gImmUnityPlugin.IMM.mPlayer.GetPlayerInfo(info);
}

extern "C" void UNITY_INTERFACE_EXPORT GetDocumentState(Player::DocumentState & state, int id)
{
	gImmUnityPlugin.IMM.mPlayer.GetDocumentState(state, id);
}

extern "C" uint32_t UNITY_INTERFACE_EXPORT GetDocumentInfoEx(int id)
{
    return gImmUnityPlugin.IMM.mPlayer.GetDocumentInfoEx(id);
}

extern "C" float UNITY_INTERFACE_EXPORT GetSound(int id)
{
    return gImmUnityPlugin.IMM.mPlayer.GetDocumentVolume(id);
}

extern "C" void UNITY_INTERFACE_EXPORT SetSound(int id, float volume)
{
    gImmUnityPlugin.IMM.mPlayer.SetDocumentVolume(id, volume);
}

extern "C" void UNITY_INTERFACE_EXPORT GetBoundingBox(int id, bound3& bound)
{
    bound = d2f(gImmUnityPlugin.IMM.mPlayer.GetDocumentBBox(id));
}

#pragma region SpawnArea

extern "C" int UNITY_INTERFACE_EXPORT GetSpawnAreaCount(int docId)
{
    return gImmUnityPlugin.IMM.mPlayer.GetSpawnAreaCount(docId);
}

extern "C" int UNITY_INTERFACE_EXPORT GetSpawnAreaList(int docId, int spawnAreaIdsSize, int* pSpawnAreaIds)
{
    const int num = gImmUnityPlugin.IMM.mPlayer.GetSpawnAreaCount(docId);
    piAssert(num <= spawnAreaIdsSize);
    for (int i = 0; i < num; ++i)
    {
        pSpawnAreaIds[i] = i;
    }
    return num;
}

extern "C" int UNITY_INTERFACE_EXPORT GetActiveSpawnAreaId(int docId)
{
    return gImmUnityPlugin.IMM.mPlayer.GetSpawnArea(docId);
}

extern "C" void UNITY_INTERFACE_EXPORT SetActiveSpawnAreaId(int docId, int activeSpawnAreaId)
{
    gImmUnityPlugin.IMM.mPlayer.SetSpawnArea(docId, activeSpawnAreaId);
}

struct SerializedSpawnArea
{
    enum class Type : uint32_t
    {
        EyeLevel = 0,
        FloorLevel = 1,
    };

    const char* mName;
    int mVersion;
    Type mType;
    bool mAnimated;
    struct Volume
    {
        enum Type
        {
            Sphere = 0,
            Box = 1,
        }type;

        struct
        {
            float r;
        }sphereExtent;
        struct
        {
            float x, y, z;
        } boxExtent;
        struct
        {
            float x, y, z;
        } offset;
    }volume;

    struct // Transform
    {
        float posx; // position
        float posy;
        float posz;
        float rotx; // rotation (quaternion)
        float roty;
        float rotz;
        float rotw;
        float sca;  // scale
    }transform;

    int locomotion;
    struct //Screenshot
    {
        uint32_t format;
        int32_t width;
        int32_t height;
        void* pData;
    }screenshot;
};

extern "C" bool UNITY_INTERFACE_EXPORT GetSpawnAreaInfo(int docId, int spawnareaId, SerializedSpawnArea& serializedSpawnArea)
{
    Document::SpawnAreaInfo spawnAreaInfo;
    if (!gImmUnityPlugin.IMM.mPlayer.GetSpawnAreaInfo(spawnAreaInfo, docId, spawnareaId))
        return false;
    serializedSpawnArea.mName = piws2str(spawnAreaInfo.mName);
    serializedSpawnArea.mVersion = spawnAreaInfo.mVersion;
    serializedSpawnArea.mType = spawnAreaInfo.mIsFloorLevel ? SerializedSpawnArea::Type::FloorLevel : SerializedSpawnArea::Type::EyeLevel;
    serializedSpawnArea.mAnimated = spawnAreaInfo.mAnimated;
    trans3d mat = (spawnAreaInfo.mSpawnAreaToWorld);
    serializedSpawnArea.transform.posx = (float)mat.mTranslation.x;
    serializedSpawnArea.transform.posy = (float)mat.mTranslation.y;
    serializedSpawnArea.transform.posz = (float)mat.mTranslation.z;
    serializedSpawnArea.transform.rotx = (float)mat.mRotation.x;
    serializedSpawnArea.transform.roty = (float)mat.mRotation.y;
    serializedSpawnArea.transform.rotz = (float)mat.mRotation.z;
    serializedSpawnArea.transform.rotw = (float)mat.mRotation.w;
    serializedSpawnArea.transform.sca = (float)mat.mScale;

    switch (spawnAreaInfo.mVolume.mType)
    {
    case LayerSpawnArea::Volume::Type::Sphere:
    {
        serializedSpawnArea.volume.type = SerializedSpawnArea::Volume::Type::Sphere;
        const vec4 sph = spawnAreaInfo.mVolume.mShape.mSphere;
        serializedSpawnArea.volume.offset.x = sph.x;
        serializedSpawnArea.volume.offset.y = sph.y;
        serializedSpawnArea.volume.offset.z = sph.z;
        serializedSpawnArea.volume.sphereExtent.r = sph.w;
    } break;
    case LayerSpawnArea::Volume::Type::Box:
    {
        serializedSpawnArea.volume.type = SerializedSpawnArea::Volume::Type::Box;
        const vec3 cen = getcenter(spawnAreaInfo.mVolume.mShape.mBox);
        const vec3 ext = getradiius(spawnAreaInfo.mVolume.mShape.mBox);
        serializedSpawnArea.volume.offset.x = cen.x;
        serializedSpawnArea.volume.offset.y = cen.y;
        serializedSpawnArea.volume.offset.z = cen.z;
        serializedSpawnArea.volume.boxExtent.x = ext.x;
        serializedSpawnArea.volume.boxExtent.y = ext.y;
        serializedSpawnArea.volume.boxExtent.z = ext.z;
    } break;
    default: { piAssert(false); } break;
    }

    serializedSpawnArea.locomotion =
        (((spawnAreaInfo.mVolume.mAllowTranslationX ? 1 : 0) << 2) |
         ((spawnAreaInfo.mVolume.mAllowTranslationY ? 1 : 0) << 1) |
         ((spawnAreaInfo.mVolume.mAllowTranslationZ ? 1 : 0) << 0));


    const piImage* pScreenshot = gImmUnityPlugin.IMM.mPlayer.GetSpawnAreaScreenshot(docId, spawnareaId);
    if (pScreenshot==nullptr)
    {
        serializedSpawnArea.screenshot.height = 0;
        serializedSpawnArea.screenshot.width = 0;
        serializedSpawnArea.screenshot.pData = nullptr;
        serializedSpawnArea.screenshot.format = 0;
    }
    else if (pScreenshot->GetNumChannels()==1)
    {
        serializedSpawnArea.screenshot.format = pScreenshot->GetFormat(0);
        serializedSpawnArea.screenshot.width = pScreenshot->GetXRes();
        serializedSpawnArea.screenshot.height = pScreenshot->GetYRes();
        serializedSpawnArea.screenshot.pData = pScreenshot->GetData(0);
    }
    return true;
}



#pragma endregion
