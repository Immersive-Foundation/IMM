#include "GlHelpers.h"
#include "Haptics.h"
#include "Log.h"
#include "Events.h"
//#include "Quillustration.h"
#include "libImmPlayer/src/player.h"

#include "libImmCore/src/libBasics/piTimer.h"
#include "libImmCore/src/libBasics/piStr.h"
#include "libImmCore/src/libMesh/piRenderMesh.h"

#include "libImmCore/src/libSound/windows/piSoundEngineAudioSDKBackend.h"

#include "../../viewer/viewer.h"
#include "../../settings.h"

#include <cstdio>
#include <strings.h>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>                  // for prctl( PR_SET_NAME )
#include <android/window.h>             // for AWINDOW_FLAG_KEEP_SCREEN_ON
#include <android/native_window_jni.h>  // for native window JNI
#include <android_native_app_glue.h>

#include <algorithm>
#include <utility>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <android/keycodes.h>
#include <android/input.h>
#include <VrApi_Types.h>
#include <OVR_PlatformInitializeResult.h>
#include <OVR_Requests_Entitlement.h>
#include <OVR_Message.h>
#include <OVR_Platform.h>
#include <OVR_Platform_Internal.h>

#include "VrApi.h"
#include "VrApi_Helpers.h"
#include "VrApi_SystemUtils.h"
#include "VrApi_Input.h"
#include "VrApi_Types.h"

// region Config

#define ENABLE_DEBUG_PANEL 1
#define ENABLE_DEBUG_VIEWPOINT_UI 0

static const char * APP_ID = "2515021945210953";

static const int CPU_LEVEL_MAX      = 5; // Use CPU 5 during loading
static const int GPU_LEVEL_MAX      = 5;
static const int CPU_LEVEL_DEFAULT  = 2;
static const int GPU_LEVEL_DEFAULT  = 3;
static const int NUM_MULTI_SAMPLES  = 4;
// Set these to override system defaults;
static const int EYE_BUFFER_WIDTH   = 0; //1216;
static const int EYE_BUFFER_HEIGHT  = 0; //1344;

// Requested levels may change if we detect too many stale frames.
int requestedCPULevel = CPU_LEVEL_DEFAULT;
int requestedGPULevel = GPU_LEVEL_DEFAULT;

int requestedEyeBufferWidth = 0;
int requestedEyeBufferHeight = 0;

static const int IMM_BYTES_THRESHOLD_FOR_STATIC = 50 * 1024 * 1024;

// static and staticTexture have shown that they perform significantly better when rendering in mono.
static ImmPlayer::StereoMode STEREO_MODE = ImmPlayer::StereoMode::None;

typedef enum handID_t_ {
    HAND_ID_LEFT = 0,
    HAND_ID_RIGHT = 1,
    HAND_ID_COUNT = 2,
} handID_t;

typedef struct RemoteDevice_ {
    static constexpr uint32_t ID_NONE = ~(uint32_t)0;
    uint32_t mRemoteDeviceID = ID_NONE;
    uint32_t mButtonState = 0;
    handID_t mHandID = HAND_ID_COUNT;
} RemoteDevice;

RemoteDevice mRemoteDevices[handID_t::HAND_ID_COUNT];

// Enable to fix the view matrix for performance testing
// #define PERFORMANCE_TESTING 1

// TODO: Remove. Likely do not need this anymore.
static bool resetTracking = false;

// Some older Quills need eye level origin
static ovrTrackingTransform vrTrackingTransformLevel = VRAPI_TRACKING_TRANSFORM_SYSTEM_CENTER_FLOOR_LEVEL;

#define MULTI_THREADED          0
#define REDUCED_LATENCY         0

using namespace ImmImporter;
using namespace ImmCore;
using namespace ImmPlayer;
using namespace ExePlayer;

static const uint32_t RENDER_BUDGET_MICROSECONDS = 8000;

static const double MIN_SPEED = 0.001;
static const int JOYSTICK_ORIENTATION_SPEED = 1;

static const ExePlayer::Settings::Rendering::Technique DEFAULT_RENDERING_TECHNIQUE = ExePlayer::Settings::Rendering::Technique::Pretessellated;

// Back press unloads the current document and resets player state. Quit from the system UI will
// lead to onDestroy and we'll release the Quill player.
bool shutdownRequested = false;
bool didProcessUnloadOnBackPress = false;

bool isUserEntitled = false;

short framesSinceStart = -1;

// Wait at most 10 frames for a new document path before showing error.
const int waitForDocFramesMax = 10;

static std::string locale;


// Add a simple message queue for notifying the render thread of Java main thread state updates.
// Particularly we need to make sure that the quillPath doesn't change during the render loop
// execution as this can lead to crashes.
enum MessageType
{
    Unknown = -1,
    LoadImmPath = 0,
    ErrorUnknown = 1,
    ErrorDisconnected = 2,
    UpdateMetadata = 4,
    UpdateViewpoints = 5,
    UpNextSelected = 6,
};

struct Message
{
    Message(MessageType t, std::string v) : type(t), value(std::move(v)) {}
    MessageType type = MessageType::Unknown;
    std::string value = "";
};

std::vector<Message> messageQueue;
std::mutex messageQueueLock;

void sendMessage(MessageType type, std::string message)
{
    Message newMessage(type, message);
    messageQueueLock.lock();
    messageQueue.push_back(newMessage);
    messageQueueLock.unlock();
}

// endregion

// region IMM State
// IMM Player and dependencies

struct AppImmPlayerState {
    std::string quillPath = "";
    bool isDisconnected = false;
    bool loadNewDocument = false;
    ExePlayer::Settings::Rendering::Technique renderingTechnique = DEFAULT_RENDERING_TECHNIQUE;
    piCameraD playerCamera = piCameraD();
    // Allow the eye buffer scale to be passed in from the intent extras for quality testing.
    float requestedEyeBufferScale = 1.0f;
    double playerSpeed = 0.01;
    bool isFirstFrame = true;
    bool buildFlavorHeadless = true;
    bool allowDevToolsInHeadless = false;
    double startTime = 0;
    double oldTime = 0;
    ovrPosef latestLeftControllerPose = ovrPosef();
    ovrPosef latestRightControllerPose = ovrPosef();
    piRasterState rasterState[2]; // Two for left and right controller rendering
    bool interactionOccurredLastFrame = false;
    int waitingHapticsCount = 0;
    std::string playerSpawnLocation = "Default";
    ImmPlayer::Player::PerformanceInfo lastPerformanceInfo;
    uint64_t downloadedBytes = 0;
    bool pendingFoveationChange;
    int foveationLevel = 2;
    int confirmExitFrameCountdown = 0;
    bool hmdWorn = true;
    // Loading indicator
    ovrTextureSwapChain * loadingSwapChain = nullptr;
    int loadingSwapIndex = 0;
    uint64_t loadingStartFrame = 0;
    GLuint * loadingFbo = nullptr;
    struct TextTexture {
        enum Status { READY = 0, TEXTURE_UPDATED = 1, DATA_UPDATED =2 }  metaStatus = READY;
        GLuint ID = 0;              // real GL id so we can call GL commands from java
        GLuint width = 952;         // must match aspect ratio of viewpoint UI panel
        GLuint height = 858;
        GLuint mipLevels = 5;
        piTexture texture;          // piLibs texture so we can use with uiLib
        float metadataHeight = 0.0f;
    } textTexture;

};

AppImmPlayerState immPlayerState;

struct AppImmPlayer
{
    piRenderer * glesRenderer = nullptr;
    piLog * pLog = nullptr;
    piTimer * pTimer = nullptr;
    piSoundEngineBackend * soundEngineBackend = nullptr;
    ExePlayer::Viewer * viewer = nullptr;
    bool initialized = false;
};

AppImmPlayer immPlayer;

void resetQuillPlayerState()
{
    ALOGV("Reset player state");
    immPlayerState.isFirstFrame = true;
    immPlayerState.quillPath.clear();
    immPlayerState.isDisconnected = false;
    immPlayerState.playerCamera = piCameraD();
    immPlayerState.playerSpeed = 0.01;
    immPlayerState.allowDevToolsInHeadless = false;
    immPlayerState.startTime = 0;
    immPlayerState.oldTime = 0;
    immPlayerState.latestLeftControllerPose = ovrPosef();
    immPlayerState.latestRightControllerPose = ovrPosef();
    immPlayerState.renderingTechnique = DEFAULT_RENDERING_TECHNIQUE;
    immPlayerState.interactionOccurredLastFrame = false;
    immPlayerState.playerSpawnLocation = "Default";
    immPlayerState.waitingHapticsCount = 0;
    immPlayerState.confirmExitFrameCountdown = 0;

    vrapi_DestroyTextureSwapChain(immPlayerState.loadingSwapChain);
    immPlayerState.loadingSwapChain = nullptr;
    free(immPlayerState.loadingFbo);
    immPlayerState.loadingFbo = nullptr;
}

void resetQuillPlayerStateForNewDocument()
{
    ALOGV("Reset player state for new document");
    didProcessUnloadOnBackPress = false;
    immPlayerState.playerCamera = piCameraD();
    immPlayerState.playerSpeed = 0.01;
    immPlayerState.allowDevToolsInHeadless = false;
    immPlayerState.startTime = 0;
    immPlayerState.oldTime = 0;
    immPlayerState.latestLeftControllerPose = ovrPosef();
    immPlayerState.latestRightControllerPose = ovrPosef();
    immPlayerState.interactionOccurredLastFrame = false;
    immPlayerState.waitingHapticsCount = 0;
    immPlayerState.confirmExitFrameCountdown = 0;

    immPlayerState.loadingSwapChain = nullptr;
    free(immPlayerState.loadingFbo);
    immPlayerState.loadingFbo = nullptr;
}

// Quill path may be included in the Java activity intent extra QUILL_PATH.
// May be the path to the Quill authoring folder or to the .imm file.
const char * getQuillPath()
{
    return immPlayerState.quillPath.c_str();
}

// endregion

// region JNI Methods
extern "C" {

void Java_org_linuxfoundation_imm_player_MainActivity_nativeSetAssetDirectory(
        JNIEnv * jni,
        jclass clazz,
        jstring assetDirectory)
{
    const char* assetDirUtf = jni->GetStringUTFChars(assetDirectory, 0);
    ExePlayer::setAssetDirectory(assetDirUtf);
    ALOGV("nativeSetAssetDirectory %s", assetDirUtf);
    jni->ReleaseStringUTFChars(assetDirectory, assetDirUtf);
}

void Java_org_linuxfoundation_imm_player_MainActivity_nativeSetLocale(
        JNIEnv * jni,
        jclass clazz,
        jstring userLocale)
{
    const char* localeUtf = jni->GetStringUTFChars(userLocale, 0);
    locale = localeUtf;
    ALOGV("nativeSetLocale %s", localeUtf);
    jni->ReleaseStringUTFChars(userLocale, localeUtf);
}

void Java_org_linuxfoundation_imm_player_MainActivity_nativeSendMessage(
        JNIEnv * jni,
        jclass clazz,
        jstring jMessage,
        jint jMessageType)
{
    const char* messageUtf = jni->GetStringUTFChars(jMessage, 0);
    ALOGV("nativeSendMessage %i %s", (int) jMessageType, messageUtf);
    sendMessage(static_cast<MessageType>(jMessageType), std::string(messageUtf));

    jni->ReleaseStringUTFChars(jMessage, messageUtf);
}

void Java_org_linuxfoundation_imm_player_MainActivity_nativeSetQuillRenderingTechnique(
        JNIEnv * jni,
        jclass clazz,
        jint renderingTechnique)
{
    ALOGV("nativeSetQuillRenderingTechnique %d", renderingTechnique);
    immPlayerState.renderingTechnique = static_cast<ExePlayer::Settings::Rendering::Technique>(renderingTechnique);
}

void Java_org_linuxfoundation_imm_player_MainActivity_nativeSetTrackingTransformLevel(
        JNIEnv * jni,
        jclass clazz,
        jstring jTrackingLevel)
{
    const char* trackingLevelUtf = jni->GetStringUTFChars(jTrackingLevel, 0);
    ALOGV("nativeSetTrackingTransformLevel %s", trackingLevelUtf);
    bool isEyeLevel = strncasecmp(trackingLevelUtf, "eye", 3) == 0;
    vrTrackingTransformLevel = isEyeLevel ? VRAPI_TRACKING_TRANSFORM_SYSTEM_CENTER_EYE_LEVEL : VRAPI_TRACKING_TRANSFORM_SYSTEM_CENTER_FLOOR_LEVEL;
    jni->ReleaseStringUTFChars(jTrackingLevel, trackingLevelUtf);
}

void Java_org_linuxfoundation_imm_player_MainActivity_nativeSetEyeBufferScale(
        JNIEnv *jni,
        jclass clazz,
        jfloat scaleFactor)
{
    ALOGV("nativeSetEyeBufferScale %f", scaleFactor);
    immPlayerState.requestedEyeBufferScale = scaleFactor;
}

void Java_org_linuxfoundation_imm_player_MainActivity_nativeSetPlayerSpawnLocation(
        JNIEnv * jni,
        jclass clazz,
        jstring jSpawnLocation)
{
    const char* spawnLocationUtf = jni->GetStringUTFChars(jSpawnLocation, 0);
    ALOGV("nativeSetPlayerSpawnLocation %s", spawnLocationUtf);
    immPlayerState.playerSpawnLocation = std::string(spawnLocationUtf);
    jni->ReleaseStringUTFChars(jSpawnLocation, spawnLocationUtf);
}

inline void assignJString(JNIEnv *jni, jstring newString, wchar_t **oldString)
{
    if (*oldString != nullptr)
    {
        free(*oldString);
        *oldString = nullptr;
    }

    if (newString != nullptr)
    {
        const char *newStringUtf = jni->GetStringUTFChars(newString, 0);
        *oldString = pistr2ws(newStringUtf);
        jni->ReleaseStringUTFChars(newString, newStringUtf);
    }
}

void createTextTexture()
{
    // Generate a GL texture to render the text in from the java side
    GL( glGenTextures(1, &immPlayerState.textTexture.ID) );
    GL( glBindTexture(GL_TEXTURE_2D, immPlayerState.textTexture.ID) );
    GL( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8UI, immPlayerState.textTexture.width, immPlayerState.textTexture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0) );
    GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0) );
    GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, immPlayerState.textTexture.mipLevels) );
    GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
    GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR) );
    GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT) );
    GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT) );
    GL( glGenerateMipmap(GL_TEXTURE_2D) );

    // Create a piTexture wrapper so we can use it in the UILib
    immPlayerState.textTexture.texture = immPlayer.glesRenderer->CreateTextureFromID(immPlayerState.textTexture.ID, piRenderer::TextureFilter::MIPMAP);
}

void initTextTexture(ovrJava* java)
{
    JNIEnv * env = java->Env;
    jclass activityClass = env->GetObjectClass(java->ActivityObject);
    jmethodID setTextureMethodId = env->GetMethodID(activityClass, "initTextTexture", "(III)V");
    env->CallVoidMethod(java->ActivityObject, setTextureMethodId, immPlayerState.textTexture.ID, immPlayerState.textTexture.width, immPlayerState.textTexture.height);
    env->DeleteLocalRef(activityClass);
}

void updateTextTextureMetadata(ovrJava* java, const char* title, const char* artist, const char* date, const char* description)
{
    JNIEnv * env = java->Env;
    jclass activityClass = env->GetObjectClass(java->ActivityObject);
    jmethodID updateTextureMethodId = env->GetMethodID(activityClass, "updateTextTextureMetadata", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)F");
    auto p1 = env->NewStringUTF(title);
    auto p2 = env->NewStringUTF(artist);
    auto p3 = env->NewStringUTF(date);
    auto p4 = env->NewStringUTF(description);
    jfloat height = env->CallFloatMethod(java->ActivityObject, updateTextureMethodId, p1, p2, p3, p4 );
    immPlayerState.textTexture.metadataHeight = (float)height;
    env->DeleteLocalRef(p1);
    env->DeleteLocalRef(p2);
    env->DeleteLocalRef(p3);
    env->DeleteLocalRef(p4);
    env->DeleteLocalRef(activityClass);
}


void destroyTextTexture()
{
    GL( glDeleteTextures(1, &immPlayerState.textTexture.ID) );
}


void Java_org_linuxfoundation_imm_player_MainActivity_nativeSetBuildFlavorHeadless(
        JNIEnv * jni,
        jclass clazz,
        jboolean jbuildFlavorHeadless)
{
    immPlayerState.buildFlavorHeadless = jbuildFlavorHeadless;
}

} // extern "C"


// fix from john pollard: issue recenter, wait a few frames, set vrapi_SetTrackingTransform
void requestResetTracking()
{
    resetTracking = true;
}

//==============================
// WaitForDebuggerToAttach
//
// wait on the debugger... once it is attached, change waitForDebugger to false
void WaitForDebuggerToAttach3()
{
    ALOGV("Waiting for debugger to attach");
    static volatile bool waitForDebugger = true;
    while (waitForDebugger)
    {
        // put your breakpoint on the sleep to wait
        usleep(100 * 1000);
    }
}

typedef struct
{
    struct
    {
        mat4x4 mViewerToEye_Prj;
    } mEye[2];
  vec2 mResolution;
} DisplayRenderState; // slot 4

// When exiting via back press or home button press we need to clean up any per-document state as
// the activity may be reused to load a new document.
void unloadAndResetOnExit(ovrJava * java)
{
    // Cancel pending downloads.
    /*if (immPlayerState.quillPath == "http" && java != nullptr)
    {
        cancelDownload(java);
    }*/

    shutdownRequested = true;
}

bool loadQuillPath(const wchar_t * quillPath, ExePlayer::Settings::Rendering::Technique renderingTechnique)
{
    if (quillPath == nullptr || wcslen(quillPath) == 0)
    {
        return false;
    }

    if (immPlayer.viewer == nullptr)
    {
        ALOGF("Could not load Quill path");
    }
    ALOGV("loadQuillPath %ls", quillPath);

    immPlayer.glesRenderer->Report();

    ExePlayer::Settings settings;
    settings.mPlayback.mLocation.mRotation = quatd(0, 0, 0, 1);
    settings.mPlayback.mLocation.mScale = 1;
    settings.mPlayback.mLocation.mFlip = flip3::N;
    settings.mPlayback.mLocation.mTranslation = vec3d(0, 0, 0);

    const wchar_t * spawnLocation = pistr2ws(immPlayerState.playerSpawnLocation.c_str());
    settings.mPlayback.mPlayerSpawn.mLocation.InitCopyW(spawnLocation);
    free((void*)spawnLocation);

    settings.mRendering.mRenderingAPI = ExePlayer::Settings::Rendering::API::GLES;
    // Set the Quill file path in settings.
    settings.mFiles.mLoad.New(1, true);
    settings.mFiles.mLoad[0].InitCopyW(quillPath);

    // IMMs that will likely use 1.2GB+ in pretessellated should use Static to avoid memory issues.
    bool useStaticRender = immPlayerState.downloadedBytes > IMM_BYTES_THRESHOLD_FOR_STATIC;
    settings.mRendering.mRenderingTechnique = useStaticRender ? ExePlayer::Settings::Rendering::Technique::Static : renderingTechnique;

    immPlayerState.foveationLevel = 2; //useStaticRender ? 2 : 0;
    immPlayerState.pendingFoveationChange = true;

    wchar_t tmpFolder[1024];
    swprintf(tmpFolder, 1023, L"%s/tmp", ExePlayer::getAssetDirectory());

    bool success = immPlayer.viewer->Init(
        0,
        immPlayer.glesRenderer,
        immPlayer.soundEngineBackend->GetEngine(),
        immPlayer.pLog,
        immPlayer.pTimer,
        STEREO_MODE,
        &settings);
    if (success) {
      immPlayer.initialized = true;
    }


    settings.mFiles.mLoad[0].End();
    settings.mFiles.mLoad.End();

    return success;
}

void handleMessageLoadImmPath(const Message & msg)
{
    immPlayerState.quillPath = msg.value;
    immPlayerState.isDisconnected = msg.type == MessageType::ErrorDisconnected;

    const bool isDownload = immPlayerState.quillPath == "http";

    ALOGV("handleMessageLoadImmPath %s %i", msg.value.c_str(), msg.type);

    // Quill Player was previously loading / rendering a document; unload it and prepare to load new
    // quill path.
    if (immPlayer.viewer != nullptr && !immPlayerState.quillPath.empty() && !shutdownRequested)
    {
        int docID = 0;
        if (!isDownload)
        {
            ALOGV("   unloading previous document...");
            immPlayer.viewer->Unload(docID);
        }

        // Careful not to clear new document state set from intent extras.
        resetQuillPlayerStateForNewDocument();
        requestResetTracking();
        immPlayerState.loadNewDocument = true;
    }
}

void initQuillPlayer(const wchar_t * quillPath, Settings::Rendering::Technique renderingTechnique, ovrJava * java)
{
    if (immPlayer.viewer != nullptr)
    {
        ALOGW("initQuillPlayer viewer not null");
        return;
    }

    immPlayer.glesRenderer  = piRenderer::Create(piRenderer::API::GLES);

    if (!immPlayer.glesRenderer)
    {
        ALOGF("Could not create piRenderer");
    }

    wchar_t tmpFolder[1024];
    swprintf(tmpFolder, 1023, L"%s/tmp", getAssetDirectory());

    immPlayer.pLog = new piLog();
    immPlayer.pTimer = new piTimer();
    immPlayer.soundEngineBackend = piSoundEngineAudioSDKBackend::Create(immPlayer.pLog);

    //WaitForDebuggerToAttach3();
    if (!immPlayer.glesRenderer->Initialize(0, nullptr, 1, false, false, nullptr, false, nullptr))
    {
        ALOGF("Could not initialize piRenderer");
    }

    /*immPlayer.shaderManager = new MngrShaders();
    if (!immPlayer.shaderManager->Init(immPlayer.glesRenderer, immPlayer.pLog))
    {
        ALOGF("Could not Init Shader Manager");
    }*/

    piSoundEngineBackend::Configuration config;
    config.mLowLatency = true; // enable android fast-path
    if (!immPlayer.soundEngineBackend->Init(nullptr, -1, &config))
    {
        ALOGF("Can't init Audio360 soundEngineBackend");
    }

    immPlayer.viewer = new Viewer();

    // Setup analytics logging
    Viewer::OnDocumentStateChange logAnalyticsEvents = [](Player::LoadingState loadingState)
    {
        switch (loadingState)
        {
            case ImmPlayer::Player::LoadingState::Loaded:
                sendMessage(UpdateViewpoints, std::string(""));
                break;
            default:
                break;
        }
    };
    immPlayer.viewer->ReplaceLoadingStateChangeListener(logAnalyticsEvents);

    if (quillPath != nullptr && !loadQuillPath(quillPath, renderingTechnique))
    {
        ALOGW("Can't init Quill Viewer %ls", quillPath);
    }

    immPlayerState.rasterState[0] = immPlayer.glesRenderer->CreateRasterState(false, false, piRenderer::CullMode::BACK, true, false);  // left controller: single sided, flip YES
    immPlayerState.rasterState[1] = immPlayer.glesRenderer->CreateRasterState(false, true, piRenderer::CullMode::BACK, true, false);  // right controller: single sided, flip NO

#ifdef LOCALIZED_TEXT
    createTextTexture();
    initTextTexture(java);
#endif
}

void destroyQuillPlayer()
{
    ALOGW("destroyQuillPlayer");

    if (immPlayer.viewer == nullptr)
    {
        ALOGW("destroyQuillPlayer quill already null");
        return;
    }

    immPlayer.glesRenderer->DestroyRasterState(immPlayerState.rasterState[0]);
    immPlayer.glesRenderer->DestroyRasterState(immPlayerState.rasterState[1]);

    /*if (immPlayer.errorMessage != nullptr) {
        immPlayer.errorMessage->Destroy(immPlayer.glesRenderer);
        immPlayer.errorMessage->Deinit(true);
        delete immPlayer.errorMessage;
        immPlayer.errorMessage = nullptr;
    }*/

    immPlayer.viewer->Deinit();
    immPlayer.initialized = false;
    delete immPlayer.viewer;
    immPlayer.viewer = nullptr;

    immPlayer.soundEngineBackend->Deinit();
    piSoundEngineAudioSDKBackend::Destroy(immPlayer.soundEngineBackend);
    immPlayer.soundEngineBackend = nullptr;
#ifdef LOCALIZED_TEXT
    destroyTextTexture();
#endif
    immPlayer.glesRenderer->Deinitialize();
    delete immPlayer.glesRenderer;
    immPlayer.glesRenderer = nullptr;

    immPlayer.pLog->End();
    delete immPlayer.pLog;
    immPlayer.pLog = nullptr;

    immPlayer.pTimer->End();
    delete immPlayer.pTimer;
    immPlayer.pTimer = nullptr;

    // Reset Quill Player state for each Quill launch
    resetQuillPlayerState();
}

// endregion

typedef struct
{
    ovrFramebuffer  FrameBuffer[VRAPI_FRAME_LAYER_EYE_MAX];
    int             NumBuffers;
} ovrRenderer;

static void ovrRenderer_Clear( ovrRenderer * renderer )
{
    ALOGV("ovrRenderer_Clear");
    for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
    {
        ovrFramebuffer_Clear( &renderer->FrameBuffer[eye] );
    }
    renderer->NumBuffers = VRAPI_FRAME_LAYER_EYE_MAX;
}

static void ovrRenderer_Create( ovrRenderer * renderer, const ovrJava * java, const bool useMultiview )
{
    renderer->NumBuffers = useMultiview ? 1 : VRAPI_FRAME_LAYER_EYE_MAX;

    // Use defaults to support next gen headsets correctly.
    int defaultWidth = vrapi_GetSystemPropertyInt( java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH );
    int defaultHeight = vrapi_GetSystemPropertyInt( java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT );

    // Allow overriding eye buffer scale by compile time constants or by runtime state.
    requestedEyeBufferWidth = EYE_BUFFER_WIDTH != 0 ? EYE_BUFFER_WIDTH : defaultWidth;
    requestedEyeBufferHeight = EYE_BUFFER_HEIGHT != 0 ? EYE_BUFFER_HEIGHT : defaultHeight;

    int width = static_cast<int>(requestedEyeBufferWidth * immPlayerState.requestedEyeBufferScale);
    int height = static_cast<int>(requestedEyeBufferHeight * immPlayerState.requestedEyeBufferScale);

    ALOGV("ovrRenderer_Create...  multiview %s eye buffer size: %ix%i",
            useMultiview ? "enabled" : "disabled", width, height);

    // Create the frame buffers.
    for ( int eye = 0; eye < renderer->NumBuffers; eye++ )
    {
        ovrFramebuffer_Create( &renderer->FrameBuffer[eye], useMultiview,
                                VRAPI_TEXTURE_FORMAT_8888_sRGB,
                                width,
                                height,
                                NUM_MULTI_SAMPLES );
    }
}

static void ovrRenderer_Destroy( ovrRenderer * renderer )
{
    for ( int eye = 0; eye < renderer->NumBuffers; eye++ )
    {
        ovrFramebuffer_Destroy( &renderer->FrameBuffer[eye] );
    }
}

static ovrLayerProjection2 ovrRenderer_RenderDownloadProgress( ovrRenderer * renderer, const ovrJava * java, const ovrTracking2 * tracking, ovrMobile * ovr, long long frameIndex )
{
#if REDUCED_LATENCY
    // Update orientation, not position.
    ovrTracking2 updatedTracking = vrapi_GetPredictedTracking2( ovr, tracking->HeadPose.TimeInSeconds );
    updatedTracking.HeadPose.Pose.Position = tracking->HeadPose.Pose.Position;
#else
    ovrTracking2 updatedTracking = *tracking;
#endif

    const double frameTime = vrapi_GetPredictedDisplayTime(ovr, frameIndex);

    ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
    layer.HeadPose = updatedTracking.HeadPose;
    for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
    {
        ovrFramebuffer * frameBuffer = &renderer->FrameBuffer[renderer->NumBuffers == 1 ? 0 : eye];
        layer.Textures[eye].ColorSwapChain = frameBuffer->ColorTextureSwapChain;
        layer.Textures[eye].SwapChainIndex = frameBuffer->TextureSwapChainIndex;
        layer.Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection( &updatedTracking.Eye[eye].ProjectionMatrix );
    }
    layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;
    //layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;

    // Enable for PTW.
    //layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_POSITIONAL_TIME_WARP;

    // QuillPlayer needs world-to-head, head-to-left-eye, head-to-right-eye, projection-left-eye, projection-right-eye matrices.
    // world-to-head we can get from the head pose. The head-to-eye matrices we need to calculate from the view matrix.

    const mat4x4 rot = mat4x4::rotate(quat(&updatedTracking.HeadPose.Pose.Orientation.x));
    const mat4x4 tra = mat4x4::translate(-updatedTracking.HeadPose.Pose.Position.x, -updatedTracking.HeadPose.Pose.Position.y, -updatedTracking.HeadPose.Pose.Position.z);

    const mat4x4d worldToHead = f2d(transpose(rot) * tra);

    const mat4x4 headToLEye = mat4x4(updatedTracking.Eye[0].ViewMatrix.M[0]) * d2f(invert(worldToHead));
    const mat4x4 headToREye = mat4x4(updatedTracking.Eye[1].ViewMatrix.M[0]) * d2f(invert(worldToHead));

    const mat4x4 projectionLEye = mat4x4(updatedTracking.Eye[0].ProjectionMatrix.M[0]);
    const mat4x4 projectionREye = mat4x4(updatedTracking.Eye[1].ProjectionMatrix.M[0]);

    // Render the eye images. If using multiview NumBuffers=1 and we'll simultaneously render to the 2D texture array layers.
    for ( int eye = 0; eye < renderer->NumBuffers; eye++ )
    {
        // NOTE: In the non-mv case, latency can be further reduced by updating the sensor prediction
        // for each eye (updates orientation, not position)
        ovrFramebuffer *frameBuffer = &renderer->FrameBuffer[eye];
        ovrFramebuffer_SetCurrent(frameBuffer);

        GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );

        GL(glEnable(GL_SCISSOR_TEST));
        GL(glViewport(0, 0, frameBuffer->Width, frameBuffer->Height));
        GL(glScissor(0, 0, frameBuffer->Width, frameBuffer->Height));

        immPlayer.glesRenderer->SetState(piSTATE_DEPTH_CLAMP, true);
        immPlayer.glesRenderer->SetState(piSTATE_FRONT_FACE, true);
        immPlayer.glesRenderer->SetState(piSTATE_CULL_FACE, true);
        immPlayer.glesRenderer->SetState(piSTATE_ALPHA_TO_COVERAGE, false);
        immPlayer.glesRenderer->SetState(piSTATE_DEPTH_TEST, true);
        immPlayer.glesRenderer->SetState(piSTATE_BLEND, true);
        immPlayer.glesRenderer->SetBlending(0, piRenderer::BlendEquation::piBLEND_ADD, piRenderer::BlendOperations::piBLEND_SRC_ALPHA, piRenderer::BlendOperations::piBLEND_ONE_MINUS_SRC_ALPHA, piRenderer::BlendEquation::piBLEND_ADD,
                                            piRenderer::BlendOperations::piBLEND_SRC_ALPHA, piRenderer::BlendOperations::piBLEND_ONE_MINUS_SRC_ALPHA);

        GL( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );

        // Explicitly clear the border texels to black when GL_CLAMP_TO_BORDER is not available.
        if (!getGLExtensions().EXT_texture_border_clamp)
        {
            // Clear to fully opaque black.
            GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
            // bottom
            GL( glScissor( 0, 0, frameBuffer->Width, 1 ) );
            GL( glClear( GL_COLOR_BUFFER_BIT ) );
            // top
            GL( glScissor( 0, frameBuffer->Height - 1, frameBuffer->Width, 1 ) );
            GL( glClear( GL_COLOR_BUFFER_BIT ) );
            // left
            GL( glScissor( 0, 0, 1, frameBuffer->Height ) );
            GL( glClear( GL_COLOR_BUFFER_BIT ) );
            // right
            GL( glScissor( frameBuffer->Width - 1, 0, 1, frameBuffer->Height ) );
            GL( glClear( GL_COLOR_BUFFER_BIT ) );
        }

        ovrFramebuffer_Resolve( frameBuffer );
        ovrFramebuffer_Advance( frameBuffer );
    }

    ovrFramebuffer_SetNone();

    return layer;
}

static ovrLayerProjection2 ovrRenderer_RenderFrame( ovrRenderer * renderer, ovrJava * java, const ovrTracking2 * tracking, ovrMobile * ovr, long long frameIndex )
{
#if REDUCED_LATENCY
    // Update orientation, not position.
    ovrTracking2 updatedTracking = vrapi_GetPredictedTracking2( ovr, tracking->HeadPose.TimeInSeconds );
    updatedTracking.HeadPose.Pose.Position = tracking->HeadPose.Pose.Position;
#else
    ovrTracking2 updatedTracking = *tracking;
#endif




    // Handle don & doff, do not render or play sounds while doffed
    if (vrapi_GetSystemStatusInt(java, VRAPI_SYS_STATUS_MOUNTED) == VRAPI_TRUE)
    {
        if (immPlayerState.hmdWorn==false)
        {
            ALOGV("donning headset");
            immPlayerState.hmdWorn=true;
            if (immPlayer.viewer->IsPaused(0))
            {
                immPlayer.viewer->Resume(0, immPlayer.pTimer->GetTimeTicks());
            }
            resetTracking = true;
        }
    }
    else
    {
        if(immPlayerState.hmdWorn==true)
        {
            ALOGV("doffing headset");
            immPlayerState.hmdWorn=false;
            if (!immPlayer.viewer->IsPaused(0))
            {
                immPlayer.viewer->Pause(0, immPlayer.pTimer->GetTimeTicks());
            }
            // we still need to render this frame to stop sounds
        }
        else
        {
            // do not render subsequent frames
            return vrapi_DefaultLayerBlackProjection2();
        }
    }


    const double frameTime = vrapi_GetPredictedDisplayTime(ovr, frameIndex);

    if (resetTracking)
    {
        ALOGV( "resetting tracking to %s level",
                 vrTrackingTransformLevel == VRAPI_TRACKING_TRANSFORM_SYSTEM_CENTER_EYE_LEVEL ? "eye" : "floor");

        vrapi_SetTrackingSpace(ovr,
                               vrTrackingTransformLevel == VRAPI_TRACKING_TRANSFORM_SYSTEM_CENTER_EYE_LEVEL ?
                               VRAPI_TRACKING_SPACE_LOCAL : VRAPI_TRACKING_SPACE_LOCAL_FLOOR);
        resetTracking = false;
    }

    ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
    layer.HeadPose = updatedTracking.HeadPose;
    for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
    {
        ovrFramebuffer * frameBuffer = &renderer->FrameBuffer[renderer->NumBuffers == 1 ? 0 : eye];
        layer.Textures[eye].ColorSwapChain = frameBuffer->ColorTextureSwapChain;
        layer.Textures[eye].SwapChainIndex = frameBuffer->TextureSwapChainIndex;
        layer.Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection( &updatedTracking.Eye[eye].ProjectionMatrix );
    }
    layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;
    //layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;

    // Enable for PTW.
    //layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_POSITIONAL_TIME_WARP;

    // QuillPlayer needs world-to-head, head-to-left-eye, head-to-right-eye, projection-left-eye, projection-right-eye matrices.
    // world-to-head we can get from the head pose. The head-to-eye matrices we need to calculate from the view matrix.

    const mat4x4 rot = mat4x4::rotate(quat(&updatedTracking.HeadPose.Pose.Orientation.x));
    mat4x4 tra = mat4x4::translate(-updatedTracking.HeadPose.Pose.Position.x, -updatedTracking.HeadPose.Pose.Position.y, -updatedTracking.HeadPose.Pose.Position.z);


    // Head
    mat4x4d worldToHead = f2d(transpose(rot) * tra);

    mat4x4d headToLEye = f2d(mat4x4(updatedTracking.Eye[0].ViewMatrix.M[0])) * invert(worldToHead);
    mat4x4d headToREye = f2d(mat4x4(updatedTracking.Eye[1].ViewMatrix.M[0])) * invert(worldToHead);

    mat4x4 projectionLEye = mat4x4(updatedTracking.Eye[0].ProjectionMatrix.M[0]);
    mat4x4 projectionREye = mat4x4(updatedTracking.Eye[1].ProjectionMatrix.M[0]);

#if 0 // TODO build head projection matrix for both eyes
    float leftLeft, leftRight, leftUp, leftDown;
    ovrMatrix4f_ExtractFov(&updatedTracking.Eye[0].ProjectionMatrix, &leftLeft, &leftRight, &leftUp, &leftDown);
    float rightLeft, rightRight, rightUp, rightDown;
    ovrMatrix4f_ExtractFov(&updatedTracking.Eye[1].ProjectionMatrix, &rightLeft, &rightRight, &rightUp, &rightDown);
    const vec2 nearFar = getNearFar_FromPerspectiveMatrix(projectionLEye);
    mat4x4 projectionHead = mat4x4(ovrMatrix4f_CreateProjectionAsymmetricFov(leftLeft,rightRight,leftUp,leftDown,nearFar.x,nearFar.y).M[0]);
#endif


    DisplayRenderState displayRenderState;

    // Render the eye images. If using multiview NumBuffers=1 and we'll simultaneously render to the 2D texture array layers.
    for ( int eye = 0; eye < renderer->NumBuffers; eye++ )
    {
        // NOTE: In the non-mv case, latency can be further reduced by updating the sensor prediction
        // for each eye (updates orientation, not position)
        ovrFramebuffer *frameBuffer = &renderer->FrameBuffer[eye];
        ovrFramebuffer_SetCurrent(frameBuffer);

        GL(glEnable(GL_SCISSOR_TEST));
        GL(glDepthMask(GL_TRUE));
        GL(glEnable(GL_DEPTH_TEST));
        GL(glDepthFunc(GL_LEQUAL));
        GL(glEnable(GL_CULL_FACE));
        GL(glCullFace(GL_BACK));
        GL(glViewport(0, 0, frameBuffer->Width, frameBuffer->Height));
        GL(glScissor(0, 0, frameBuffer->Width, frameBuffer->Height));

        const double time = immPlayer.pTimer->GetTime() - immPlayerState.startTime;
        const float dtime = float(time - immPlayerState.oldTime);

        immPlayerState.oldTime = time;

        const ivec2 resolution(EYE_BUFFER_WIDTH, EYE_BUFFER_HEIGHT);

        // TODO: populate controller and handle input directly in Viewer.
        //  Can create multiple navigation models with a shared interface.
        piVRHMD::Controller controller;

        // For mono rendering, we'll just use the perf info from the left eye so we don't get
        // different values rendered because of asymmetric eye matrices.
        if (eye == 0)
        {
            immPlayerState.lastPerformanceInfo = immPlayer.viewer->GetPerformanceInfoForFrame();
        }

        const trans3d &transWorldToHead = fromMatrix(worldToHead);

        if (STEREO_MODE == ImmPlayer::StereoMode::None)
        {
            ImmCore::mat4x4 viewer2eyeProjection = eye == 0 ? projectionLEye * d2f(headToLEye) : projectionREye * d2f(headToREye);

#if defined(PERFORMANCE_TESTING)
            const trans3d transPlayerWorldToHead = fromMatrix(worldToHead) * fromMatrix(quillState.playerCamera.GetWorldToCamera());
#else
            const trans3d transPlayerWorldToHead = transWorldToHead * fromMatrix(immPlayerState.playerCamera.GetWorldToCamera()) ;
#endif
            if (eye == 0)
            {
                immPlayer.viewer->GlobalWork(nullptr, true, transPlayerWorldToHead, &controller,
                                             nullptr, immPlayer.pLog, dtime, resolution, true,
                                             RENDER_BUDGET_MICROSECONDS, immPlayerState.isFirstFrame);
            }

            immPlayer.viewer->GlobalRender(transPlayerWorldToHead, viewer2eyeProjection);
            immPlayer.viewer->RenderMono(resolution, transPlayerWorldToHead, eye);
        }
        else
        {
            const trans3d transPlayerWorldToHead = transWorldToHead * fromMatrix(immPlayerState.playerCamera.GetWorldToCamera());

            if (STEREO_MODE == ImmPlayer::StereoMode::Fallback)
            {
                mat4x4d headToEye = eye == 0 ? headToLEye : headToREye;
                mat4x4 eyeProjection = eye == 0 ? projectionLEye : projectionREye;

                if (eye == 0)
                {
                    immPlayer.viewer->GlobalWork(nullptr, true, transPlayerWorldToHead, &controller, nullptr, immPlayer.pLog, dtime, resolution, true, RENDER_BUDGET_MICROSECONDS, immPlayerState.isFirstFrame);
                    immPlayer.viewer->GlobalRender(transPlayerWorldToHead, projectionLEye); // TODO schevrel: this should be head projection
                }

                immPlayer.viewer->RenderStereoMultiPass(resolution, eye, headToEye, eyeProjection, transPlayerWorldToHead );
            }
            else if (STEREO_MODE == ImmPlayer::StereoMode::Preferred)
            {
                immPlayer.viewer->GlobalWork(nullptr, true, transPlayerWorldToHead, &controller, nullptr, immPlayer.pLog, dtime, resolution, true, RENDER_BUDGET_MICROSECONDS, immPlayerState.isFirstFrame);
                immPlayer.viewer->GlobalRender(transPlayerWorldToHead, projectionLEye);  // TODO schevrel: this should be head projection
                immPlayer.viewer->RenderStereoSinglePass(resolution, transPlayerWorldToHead, headToLEye, projectionLEye, headToREye, projectionREye, nullptr);
            }
        }

        // Explicitly clear the border texels to black when GL_CLAMP_TO_BORDER is not available.
        if (!getGLExtensions().EXT_texture_border_clamp)
        {
            // Clear to fully opaque black.
            GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
            // bottom
            GL( glScissor( 0, 0, frameBuffer->Width, 1 ) );
            GL( glClear( GL_COLOR_BUFFER_BIT ) );
            // top
            GL( glScissor( 0, frameBuffer->Height - 1, frameBuffer->Width, 1 ) );
            GL( glClear( GL_COLOR_BUFFER_BIT ) );
            // left
            GL( glScissor( 0, 0, 1, frameBuffer->Height ) );
            GL( glClear( GL_COLOR_BUFFER_BIT ) );
            // right
            GL( glScissor( frameBuffer->Width - 1, 0, 1, frameBuffer->Height ) );
            GL( glClear( GL_COLOR_BUFFER_BIT ) );
        }

        ovrFramebuffer_Resolve( frameBuffer );
        ovrFramebuffer_Advance( frameBuffer );
    }

    ovrFramebuffer_SetNone();

    if (immPlayer.soundEngineBackend != nullptr)
        immPlayer.soundEngineBackend->Tick();

    return layer;
}

static ovrLayerLoadingIcon2 ovrRenderer_RenderTimewarpLoadingIcon(const long long frameIndex)
{
    // Show a loading icon until the file is loaded.
    ovrLayerLoadingIcon2 iconLayer = vrapi_DefaultLayerLoadingIcon2();

    const int frameCount = 52;

    if (immPlayerState.loadingSwapChain == nullptr)
    {
        immPlayerState.loadingSwapChain = vrapi_CreateTextureSwapChain(
                VRAPI_TEXTURE_TYPE_2D,
                VRAPI_TEXTURE_FORMAT_8888,
                64, 64,
                1,
                true );
    }

    const int length = vrapi_GetTextureSwapChainLength(immPlayerState.loadingSwapChain );

    if (immPlayerState.loadingFbo == nullptr)
    {
        immPlayerState.loadingFbo = (GLuint*)malloc(length*sizeof(GLuint));
        glGenFramebuffers(length, immPlayerState.loadingFbo );
    }

    if (immPlayerState.loadingStartFrame == 0)
    {
        immPlayerState.loadingStartFrame = frameIndex;
    }

    iconLayer.ColorSwapChain = immPlayerState.loadingSwapChain;
    iconLayer.SpinSpeed = 0.0;
    iconLayer.SwapChainIndex = immPlayerState.loadingSwapIndex;
    iconLayer.SpinScale = 16.0f;


    return iconLayer;
}

// endregion

// region ovrRenderThread (#if MULTI_THREADED)

#if MULTI_THREADED

typedef enum
{
    RENDER_FRAME,
    RENDER_LOADING_ICON,
    RENDER_BLACK_FINAL
} ovrRenderType;

typedef struct
{
    JavaVM *            JavaVm;
    jobject             ActivityObject;
    const ovrEgl *      ShareEgl;
    pthread_t           Thread;
    int                 Tid;
    bool                UseMultiview;
    // Synchronization
    bool                Exit;
    bool                WorkAvailableFlag;
    bool                WorkDoneFlag;
    pthread_cond_t      WorkAvailableCondition;
    pthread_cond_t      WorkDoneCondition;
    pthread_mutex_t     Mutex;
    // Latched data for rendering.
    ovrMobile *         Ovr;
    ovrRenderType       RenderType;
    long long           FrameIndex;
    double              DisplayTime;
    int                 SwapInterval;
    ovrTracking2        Tracking;
} ovrRenderThread;

void * RenderThreadFunction( void * parm )
{
    ovrRenderThread * renderThread = (ovrRenderThread *)parm;
    renderThread->Tid = gettid();

    ovrJava java;
    java.Vm = renderThread->JavaVm;
    (java.Vm)->AttachCurrentThread( &java.Env, NULL );
    java.ActivityObject = renderThread->ActivityObject;

    // Note that AttachCurrentThread will reset the thread name.
    prctl( PR_SET_NAME, (long)"OVR::Renderer", 0, 0, 0 );

    ovrEgl egl;

    ovrEgl_CreateContext( &egl, renderThread->ShareEgl );

    ovrRenderer renderer;
    ovrRenderer_Create( &renderer, &java, renderThread->UseMultiview );

    for( ; ; )
    {
        // Signal work completed.
        pthread_mutex_lock( &renderThread->Mutex );
        renderThread->WorkDoneFlag = true;
        pthread_cond_signal( &renderThread->WorkDoneCondition );
        pthread_mutex_unlock( &renderThread->Mutex );

        // Wait for work.
        pthread_mutex_lock( &renderThread->Mutex );
        while ( !renderThread->WorkAvailableFlag )
        {
            pthread_cond_wait( &renderThread->WorkAvailableCondition, &renderThread->Mutex );
        }
        renderThread->WorkAvailableFlag = false;
        pthread_mutex_unlock( &renderThread->Mutex );

        // Check for exit.
        if ( renderThread->Exit )
        {
            break;
        }

        // Render.
        ovrLayer_Union2 layers[ovrMaxLayerCount] = {};
        int layerCount = 0;
        int frameFlags = 0;

        if ( renderThread->RenderType == RENDER_FRAME )
        {
            ovrLayerProjection2 layer;
            layer = ovrRenderer_RenderFrame( &renderer, &java,
                    &renderThread->Tracking, renderThread->Ovr );

            layers[layerCount++].Projection = layer;
        }
        else if ( renderThread->RenderType == RENDER_LOADING_ICON )
        {
            ovrLayerProjection2 blackLayer = vrapi_DefaultLayerBlackProjection2();
            layers[layerCount++].Projection = blackLayer;

            ovrLayerLoadingIcon2 iconLayer = vrapi_DefaultLayerLoadingIcon2();
            layers[layerCount++].LoadingIcon = iconLayer;

            frameFlags |= VRAPI_FRAME_FLAG_FLUSH;
        }
        else if ( renderThread->RenderType == RENDER_BLACK_FINAL )
        {
            ovrLayerProjection2 layer = vrapi_DefaultLayerBlackProjection2();
            layers[layerCount++].Projection = layer;

            frameFlags |= VRAPI_FRAME_FLAG_FLUSH | VRAPI_FRAME_FLAG_FINAL;
        }

        const ovrLayerHeader2 * layerList[ovrMaxLayerCount] = {};
        for ( int i = 0; i < layerCount; i++ )
        {
            layerList[i] = &layers[i].Header;
        }

        ovrSubmitFrameDescription2 frameDesc = {};
        frameDesc.Flags = frameFlags;
        frameDesc.SwapInterval = renderThread->SwapInterval;
        frameDesc.FrameIndex = renderThread->FrameIndex;
        frameDesc.DisplayTime = renderThread->DisplayTime;
        frameDesc.LayerCount = layerCount;
        frameDesc.Layers = layerList;

        vrapi_SubmitFrame2( renderThread->Ovr, &frameDesc );
    }

    ovrRenderer_Destroy( &renderer );
    ovrEgl_DestroyContext( &egl );

    (java.Vm)->DetachCurrentThread();

    return NULL;
}

static void ovrRenderThread_Clear( ovrRenderThread * renderThread )
{
    renderThread->JavaVm = NULL;
    renderThread->ActivityObject = NULL;
    renderThread->ShareEgl = NULL;
    renderThread->Thread = 0;
    renderThread->Tid = 0;
    renderThread->UseMultiview = false;
    renderThread->Exit = false;
    renderThread->WorkAvailableFlag = false;
    renderThread->WorkDoneFlag = false;
    renderThread->Ovr = NULL;
    renderThread->RenderType = RENDER_FRAME;
    renderThread->FrameIndex = 1;
    renderThread->DisplayTime = 0;
    renderThread->SwapInterval = 1;
    renderThread->Scene = NULL;
}

static void ovrRenderThread_Create( ovrRenderThread * renderThread, const ovrJava * java,
                                    const ovrEgl * shareEgl, const bool useMultiview )
{
    renderThread->JavaVm = java->Vm;
    renderThread->ActivityObject = java->ActivityObject;
    renderThread->ShareEgl = shareEgl;
    renderThread->Thread = 0;
    renderThread->Tid = 0;
    renderThread->UseMultiview = useMultiview;
    renderThread->Exit = false;
    renderThread->WorkAvailableFlag = false;
    renderThread->WorkDoneFlag = false;
    pthread_cond_init( &renderThread->WorkAvailableCondition, NULL );
    pthread_cond_init( &renderThread->WorkDoneCondition, NULL );
    pthread_mutex_init( &renderThread->Mutex, NULL );

    const int createErr = pthread_create( &renderThread->Thread, NULL, RenderThreadFunction, renderThread );
    if ( createErr != 0 )
    {
        ALOGF( "pthread_create returned %i", createErr );
    }
}

static void ovrRenderThread_Destroy( ovrRenderThread * renderThread )
{
    pthread_mutex_lock( &renderThread->Mutex );
    renderThread->Exit = true;
    renderThread->WorkAvailableFlag = true;
    pthread_cond_signal( &renderThread->WorkAvailableCondition );
    pthread_mutex_unlock( &renderThread->Mutex );

    pthread_join( renderThread->Thread, NULL );
    pthread_cond_destroy( &renderThread->WorkAvailableCondition );
    pthread_cond_destroy( &renderThread->WorkDoneCondition );
    pthread_mutex_destroy( &renderThread->Mutex );
}

static void ovrRenderThread_Submit( ovrRenderThread * renderThread, ovrMobile * ovr,
        ovrRenderType type, long long frameIndex, double displayTime, int swapInterval,
        const ovrTracking2 * tracking )
{
    // Wait for the renderer thread to finish the last frame.
    pthread_mutex_lock( &renderThread->Mutex );
    while ( !renderThread->WorkDoneFlag )
    {
        pthread_cond_wait( &renderThread->WorkDoneCondition, &renderThread->Mutex );
    }
    renderThread->WorkDoneFlag = false;
    // Latch the render data.
    renderThread->Ovr = ovr;
    renderThread->RenderType = type;
    renderThread->FrameIndex = frameIndex;
    renderThread->DisplayTime = displayTime;
    renderThread->SwapInterval = swapInterval;
    if ( tracking != NULL )
    {
        renderThread->Tracking = *tracking;
    }
    // Signal work is available.
    renderThread->WorkAvailableFlag = true;
    pthread_cond_signal( &renderThread->WorkAvailableCondition );
    pthread_mutex_unlock( &renderThread->Mutex );
}

static void ovrRenderThread_Wait( ovrRenderThread * renderThread )
{
    // Wait for the renderer thread to finish the last frame.
    pthread_mutex_lock( &renderThread->Mutex );
    while ( !renderThread->WorkDoneFlag )
    {
        pthread_cond_wait( &renderThread->WorkDoneCondition, &renderThread->Mutex );
    }
    pthread_mutex_unlock( &renderThread->Mutex );
}

static int ovrRenderThread_GetTid( ovrRenderThread * renderThread )
{
    ovrRenderThread_Wait( renderThread );
    return renderThread->Tid;
}

#endif // MULTI_THREADED

// endregion

// region ovrApp

typedef struct
{
    ovrJava             Java;
    ovrEgl              Egl;
    ANativeWindow *     NativeWindow;
    bool                Resumed;
    ovrMobile *         Ovr;
    long long           FrameIndex;
    double              DisplayTime;
    int                 SwapInterval;
    ovrPerformanceParms PerformanceParms;
    bool                BackButtonDownLastFrame;
#if MULTI_THREADED
    ovrRenderThread     RenderThread;
#else
    ovrRenderer         Renderer;
#endif
    bool                UseMultiview;
} ovrApp;

static void ovrApp_Clear( ovrApp * app )
{
    app->Java.Vm = NULL;
    app->Java.Env = NULL;
    app->Java.ActivityObject = NULL;
    app->NativeWindow = NULL;
    app->Resumed = false;
    app->Ovr = NULL;
    app->FrameIndex = 1;
    app->DisplayTime = 0;
    app->SwapInterval = 1;
    app->PerformanceParms = vrapi_DefaultPerformanceParms();
    app->BackButtonDownLastFrame = false;
    app->UseMultiview = false;

    ovrEgl_Clear( &app->Egl );
#if MULTI_THREADED
    ovrRenderThread_Clear( &app->RenderThread );
#else
    ovrRenderer_Clear( &app->Renderer );
#endif
}

static void ovrApp_PushBlackFinal( ovrApp * app )
{
#if MULTI_THREADED
    ovrRenderThread_Submit( &app->RenderThread, app->Ovr,
            RENDER_BLACK_FINAL, app->FrameIndex, app->DisplayTime, app->SwapInterval,
            NULL, NULL, NULL );
#else
    int frameFlags = 0;
    frameFlags |= VRAPI_FRAME_FLAG_FLUSH | VRAPI_FRAME_FLAG_FINAL;

    ovrLayerProjection2 layer = vrapi_DefaultLayerBlackProjection2();

    const ovrLayerHeader2 * layers[] =
            {
                    &layer.Header
            };

    ovrSubmitFrameDescription2 frameDesc = {};
    frameDesc.Flags = frameFlags;
    frameDesc.SwapInterval = 1;
    frameDesc.FrameIndex = app->FrameIndex;
    frameDesc.DisplayTime = app->DisplayTime;
    frameDesc.LayerCount = 1;
    frameDesc.Layers = layers;

    vrapi_SubmitFrame2( app->Ovr, &frameDesc );
#endif
}

static void ovrApp_HandleVrModeChanges( ovrApp * app )
{
    ALOGV( "        vrapi_HandleVrModeChange()" );
    if ( app->Resumed != false && app->NativeWindow != NULL )
    {
        if ( app->Ovr == NULL )
        {
            ovrModeParms parms = vrapi_DefaultModeParms( &app->Java );
            // No need to reset the FLAG_FULLSCREEN window flag when using a View
            parms.Flags &= ~VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;
            // We definitely want high quality sRGB filtering.
            // parms.Flags |= VRAPI_MODE_FLAG_FRONT_BUFFER_SRGB;
            // parms.Flags |= VRAPI_MODE_FLAG_FRONT_BUFFER_565;
            parms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
            parms.Display = (size_t)app->Egl.Display;
            parms.WindowSurface = (size_t)app->NativeWindow;
            parms.ShareContext = (size_t)app->Egl.Context;

            ALOGV( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );

            ALOGV( "        vrapi_EnterVrMode()" );

            app->Ovr = vrapi_EnterVrMode( &parms );

            ALOGV( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );

            // If entering VR mode failed then the ANativeWindow was not valid.
            if ( app->Ovr == NULL )
            {
                ALOGF( "Invalid ANativeWindow!" );
                app->NativeWindow = NULL;
            }

            // Set performance parameters once we have entered VR mode and have a valid ovrMobile.
            if ( app->Ovr != NULL )
            {
                vrapi_SetClockLevels( app->Ovr, app->PerformanceParms.CpuLevel, app->PerformanceParms.GpuLevel );

                ALOGV( "        vrapi_SetClockLevels( %d, %d )", app->PerformanceParms.CpuLevel, app->PerformanceParms.GpuLevel );

                vrapi_SetPerfThread( app->Ovr, VRAPI_PERF_THREAD_TYPE_MAIN, app->PerformanceParms.MainThreadTid );

                ALOGV( "        vrapi_SetPerfThread( MAIN, %d )", app->PerformanceParms.MainThreadTid );

                vrapi_SetPerfThread( app->Ovr, VRAPI_PERF_THREAD_TYPE_RENDERER, app->PerformanceParms.RenderThreadTid );

                ALOGV( "        vrapi_SetPerfThread( RENDERER, %d )", app->PerformanceParms.RenderThreadTid );

                // Use fixed foveated rendering with TBR.
                vrapi_SetPropertyInt( &app->Java, VRAPI_FOVEATION_LEVEL, 2);

                // Use extra latency mode.
                vrapi_SetExtraLatencyMode( app->Ovr, VRAPI_EXTRA_LATENCY_MODE_ON );
            }
        }

        requestResetTracking();
    }
    else
    {
        if ( app->Ovr != NULL )
        {
#if MULTI_THREADED
            // Make sure the renderer thread is no longer using the ovrMobile.
            ovrRenderThread_Wait( &app->RenderThread );
#endif
            ALOGV( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );

            ALOGV( "        vrapi_LeaveVrMode()" );

            vrapi_LeaveVrMode( app->Ovr );
            app->Ovr = NULL;

            ALOGV( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );
        }
    }
}

static void ovrApp_HandleInput(ovrApp * app, ANativeActivity * activity)
{
    if (shutdownRequested)
    {
        return;
    }

    bool backButtonDownThisFrame = false;
    vec3d moveDistance = vec3d(0.0, 0.0, 0.0);

    int orientationDelta = 0;

    bool enableDevTools = immPlayerState.allowDevToolsInHeadless || !immPlayerState.buildFlavorHeadless;

    bool gripTriggersAndJoystickButtonsPressed = true;
    bool gripTriggersAndJoystickButtonsPressedLastFrame = true;

    for (uint32_t deviceIndex = 0; ; deviceIndex++)
    {
        ovrInputCapabilityHeader capsHeader;
        ovrResult result = vrapi_EnumerateInputDevices( app->Ovr, deviceIndex, &capsHeader );
        if ( result < 0 )
        {
            break;
        }

        if ( capsHeader.Type == ovrControllerType_TrackedRemote )
        {
            ovrInputStateTrackedRemote trackedRemoteState;
            trackedRemoteState.Header.ControllerType = ovrControllerType_TrackedRemote;
            result = vrapi_GetCurrentInputState( app->Ovr, capsHeader.DeviceID, &trackedRemoteState.Header );

            if ( result != ovrSuccess )
            {
                ALOGW("Error(%i): Failed to get input state for hand %i", result, deviceIndex);
                continue;
            }

            backButtonDownThisFrame |= ( ( trackedRemoteState.Buttons & ovrButton_B ) != 0 || ( trackedRemoteState.Buttons & ovrButton_Back ) != 0  );

            ovrInputTrackedRemoteCapabilities remoteCapabilities;
            remoteCapabilities.Header = capsHeader;
            result = vrapi_GetInputDeviceCapabilities( app->Ovr, &remoteCapabilities.Header );

            if ( result != ovrSuccess )
            {
                ALOGW("Error(%i): Failed to get input device capabilities for hand %i", result, deviceIndex);
                continue;
            }

            // Compare with last frame's button state for this hand's controller.
            uint32_t buttonsPressed = trackedRemoteState.Buttons & ~mRemoteDevices[deviceIndex].mButtonState;
            uint32_t buttonsReleased = ~trackedRemoteState.Buttons & mRemoteDevices[deviceIndex].mButtonState;

            bool isLeftHand = (remoteCapabilities.ControllerCapabilities & ovrControllerCaps_LeftHand) != 0;
            bool isRightHand = (remoteCapabilities.ControllerCapabilities & ovrControllerCaps_RightHand) != 0;

            gripTriggersAndJoystickButtonsPressed &= (trackedRemoteState.Buttons & ovrButton_GripTrigger) != 0 &&
                    (trackedRemoteState.Buttons & ovrButton_Joystick) != 0;

            gripTriggersAndJoystickButtonsPressedLastFrame &= (mRemoteDevices[deviceIndex].mButtonState & ovrButton_GripTrigger) != 0 &&
                    (mRemoteDevices[deviceIndex].mButtonState & ovrButton_Joystick) != 0;

            if (mRemoteDevices[deviceIndex].mRemoteDeviceID == RemoteDevice::ID_NONE)
            {
                mRemoteDevices[deviceIndex].mRemoteDeviceID = capsHeader.DeviceID;
                mRemoteDevices[deviceIndex].mHandID = isLeftHand ? HAND_ID_LEFT : (isRightHand ? HAND_ID_RIGHT : HAND_ID_COUNT);
            }
            else if (mRemoteDevices[deviceIndex].mRemoteDeviceID != capsHeader.DeviceID)
            {
                ALOGW("Expected device ID %i for hand %i instead got %i",
                         mRemoteDevices[deviceIndex].mRemoteDeviceID, deviceIndex, capsHeader.DeviceID);
            }

            ovrTracking remoteTracking;
            result = vrapi_GetInputTrackingState( app->Ovr, capsHeader.DeviceID, app->DisplayTime, &remoteTracking );

            if ( result != ovrSuccess )
            {
                ALOGW("Could not get input tracking state for device %i", capsHeader.DeviceID);
            }

            // Save the controller pose for rendering.
            if (isLeftHand)
            {
                immPlayerState.latestLeftControllerPose = remoteTracking.HeadPose.Pose;
            }
            else if (isRightHand)
            {
                immPlayerState.latestRightControllerPose = remoteTracking.HeadPose.Pose;
            }

            mRemoteDevices[deviceIndex].mButtonState = trackedRemoteState.Buttons;

            // Dual joystick based player movement. Right joystick handles forward/back with Y and
            // left/right with X. Left joystick handles up/down with Y. Left/Right joystick press to
            // decrease/increase speed respectively.

            if ( enableDevTools && (remoteCapabilities.ControllerCapabilities & ovrControllerCaps_HasJoystick) )
            {
                if ( isLeftHand )
                {
                    if ( buttonsReleased & ovrButton_Joystick )
                    {
                        immPlayerState.playerSpeed = std::max(immPlayerState.playerSpeed /= 2.0,
                                                              MIN_SPEED);
                        ALOGV("Reduced speed to %.4f", immPlayerState.playerSpeed);
                    }

                    if (trackedRemoteState.Joystick.x > 0.9)
                    {
                        orientationDelta = JOYSTICK_ORIENTATION_SPEED;
                    }
                    else if (trackedRemoteState.Joystick.x < -0.9)
                    {
                        orientationDelta = -JOYSTICK_ORIENTATION_SPEED;
                    }
                    else
                    {
                        // Only move vertically if no horizontal movement is detected.
                        moveDistance.y += trackedRemoteState.Joystick.y;
                    }
                }
                else if (isRightHand)
                {
                    if ( buttonsReleased & ovrButton_Joystick )
                    {
                        immPlayerState.playerSpeed *= 2.0;
                        ALOGV("Increased speed to %.4f", immPlayerState.playerSpeed);
                    }

                    moveDistance.x += trackedRemoteState.Joystick.x;
                    moveDistance.z -= trackedRemoteState.Joystick.y;
                }
            }

            if ( buttonsReleased & ovrButton_Y )
            {
                ALOGV("Y button released");
            }

            if ( buttonsReleased & ovrButton_GripTrigger )
            {
                ALOGV(" %s Grip trigger released", isLeftHand ? "Left" : "Right");
            }

            if (buttonsReleased & ovrButton_A )
            {
                const int docId = 0;
                if (immPlayer.viewer != nullptr && immPlayer.viewer->IsDocumentLoaded(docId))
                {
                    const int documentId = 0;
                    if (immPlayer.viewer->IsWaiting(documentId))
                        immPlayer.viewer->Continue(documentId);
                }
            }
        }

    }

    // Secret combo to allow joystick controls and performance panel in headless (production) Quill
    // Player build flavor. Left and Right grip triggers and joystick buttons all pressed at the
    // same time.
    if (immPlayerState.buildFlavorHeadless &&
        !gripTriggersAndJoystickButtonsPressedLastFrame &&
        gripTriggersAndJoystickButtonsPressed)
    {
        immPlayerState.allowDevToolsInHeadless = !immPlayerState.allowDevToolsInHeadless;
        ALOGV("%s dev tools", immPlayerState.allowDevToolsInHeadless ? "enabling" : "disabling");
        if (!immPlayerState.allowDevToolsInHeadless)
        {
            immPlayerState.playerCamera = piCameraD();
        }
    }

    const double predictedDisplayTime = vrapi_GetPredictedDisplayTime( app->Ovr, app->FrameIndex );

    const ovrTracking2 tracking = vrapi_GetPredictedTracking2( app->Ovr, predictedDisplayTime );

    mat3x3d orientation = mat3x3d::rotate(quatd(tracking.HeadPose.Pose.Orientation.x,
                                                tracking.HeadPose.Pose.Orientation.y,
                                                tracking.HeadPose.Pose.Orientation.z,
                                                tracking.HeadPose.Pose.Orientation.w));
    immPlayerState.playerCamera.LocalMove(immPlayerState.playerSpeed * (orientation * moveDistance));
    immPlayerState.playerCamera.RotateXY(static_cast<double>(orientationDelta) * M_PI / 180.0, 0);

    bool backButtonDownLastFrame = app->BackButtonDownLastFrame;
    app->BackButtonDownLastFrame = backButtonDownThisFrame;

    const int docID = 0;
    // The document can't process an unload command until loading stage is finished. So we'll wait
    // till load is completed to register the back press.
    bool isLoading = immPlayer.viewer != nullptr && immPlayer.viewer->IsDocumentLoading(docID);
    // If back button is pressed we need to wait one frame to issue the document unload.
    if ( backButtonDownLastFrame && !backButtonDownThisFrame && !isLoading)
    {
        bool shouldConfirmExit = immPlayer.viewer->IsDocumentLoaded(docID) && immPlayer.viewer->GetChapterCount(docID) > 1;
        if (shouldConfirmExit && immPlayerState.confirmExitFrameCountdown == 0)
        {
            // Show the confirm dialog for ~3 seconds
            immPlayerState.confirmExitFrameCountdown = 216;
        }
        else
        {
            ALOGV( "back button short press" );
            ALOGV( "unloading document... shutdown next frame" );
            unloadAndResetOnExit(&app->Java);
        }
    }
}

// endregion

// region Android Native Activity

void tryPauseQuillPlayer()
{
    int docID = 0;
    if (immPlayer.viewer != nullptr && immPlayer.viewer->IsDocumentLoaded(docID) && !immPlayer.viewer->IsPaused(docID))
    {
        ALOGV("pausing quill player");
        immPlayer.viewer->Pause(docID, immPlayer.pTimer->GetTimeTicks());
    }
}

void tryResumeQuillPlayer()
{
    int docID = 0;
    if (immPlayer.viewer != nullptr && immPlayer.viewer->IsDocumentLoaded(docID) && !immPlayer.viewer->IsPaused(docID))
    {
        ALOGV("resuming quill player");
        immPlayer.viewer->Resume(docID, immPlayer.pTimer->GetTimeTicks());
    }
}

/**
 * Process the next main command.
 */
static void app_handle_cmd( struct android_app * app, int32_t cmd )
{
    ovrApp * appState = (ovrApp *)app->userData;

    switch ( cmd )
    {
        // There is no APP_CMD_CREATE. The ANativeActivity creates the
        // application thread from onCreate(). The application thread
        // then calls android_main().
        case APP_CMD_START:
        {
            ALOGV( "onStart()" );
            ALOGV( "    APP_CMD_START" );
            shutdownRequested = false;
            didProcessUnloadOnBackPress = false;
            framesSinceStart = -1;
            break;
        }
        case APP_CMD_RESUME:
        {
            ALOGV( "onResume()" );
            ALOGV( "    APP_CMD_RESUME" );
            appState->Resumed = true;
            tryResumeQuillPlayer();
            break;
        }
        case APP_CMD_PAUSE:
        {
            ALOGV( "onPause()" );
            ALOGV( "    APP_CMD_PAUSE" );
            appState->Resumed = false;
            tryPauseQuillPlayer();
            break;
        }
        case APP_CMD_STOP:
        {
            ALOGV( "onStop()" );
            ALOGV( "    APP_CMD_STOP" );
            break;
        }
        case APP_CMD_DESTROY:
        {
            ALOGV( "onDestroy()" );
            ALOGV( "    APP_CMD_DESTROY" );
            appState->NativeWindow = NULL;

            shutdownRequested = true;
            break;
        }
        case APP_CMD_INIT_WINDOW:
        {
            ALOGV( "surfaceCreated()" );
            ALOGV( "    APP_CMD_INIT_WINDOW" );
            appState->NativeWindow = app->window;
            break;
        }
        case APP_CMD_TERM_WINDOW:
        {
            ALOGV( "surfaceDestroyed()" );
            ALOGV( "    APP_CMD_TERM_WINDOW" );
            appState->NativeWindow = NULL;
            break;
        }
    }
}

static void ovrApp_PushBlackFinal( ovrApp * app );

void processOVRMessages(const android_app * app, ovrJava * java)
{
    if (!immPlayerState.buildFlavorHeadless)
        return;

    ovrMessageHandle message;
    while ((message = ovr_PopMessage()) != nullptr)
    {
        switch (ovr_Message_GetType(message))
        {
            case ovrMessage_User_GetAccessToken:
                if (!ovr_Message_IsError(message))
                {
                    ALOGV("Received access token.");
                    //reportAccessToken(java, ovr_Message_GetString(message), 1);
                    ovr_FreeMessage(message);
                }
                else
                {
                    ovrErrorHandle pError = ovr_Message_GetError(message);
                    ALOGE("Access token request failed: %s", ovr_Error_GetMessage(pError));
                    //reportAccessToken(java, nullptr, 0);
                    ovr_FreeMessage(message);
                }
                break;

#if !defined(DEBUG)
            case ovrMessage_Entitlement_GetIsViewerEntitled:
                if (!immPlayerState.buildFlavorHeadless || isUserEntitled)
                {
                    return;
                }

                if (!ovr_Message_IsError(message))
                {
                    ALOGV("Entitlement check passed.");
                    // User is entitled.  Continue with normal game behaviour
                    ovr_FreeMessage(message);
                    isUserEntitled = true;
                }
                else
                {
                    ALOGE("Entitlement check failed! Received Error checking if user is entitled: %s",
                          ovr_Message_GetString(message));
                    ovr_FreeMessage(message);
                    ANativeActivity_finish(app->activity);
                }
                break;
#endif

            default:
                break;
        }
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main( struct android_app * app )
{
    ALOGV( "----------------------------------------------------------------" );
    ALOGV( "android_app_entry()" );
    ALOGV( "    android_main()" );

    ANativeActivity_setWindowFlags( app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0 );

    ovrJava java;
    java.Vm = app->activity->vm;
    (java.Vm)->AttachCurrentThread( &java.Env, NULL );
    java.ActivityObject = app->activity->clazz;

    // Note that AttachCurrentThread will reset the thread name.
    prctl( PR_SET_NAME, (long)"OVR::Main", 0, 0, 0 );

    const ovrInitParms initParms = vrapi_DefaultInitParms( &java );
    int32_t initResult = vrapi_Initialize( &initParms );
    if ( initResult != VRAPI_INITIALIZE_SUCCESS )
    {
        // NOTE - Use _Exit, which doesn't call static destructors. Calling static dtor's can be highly
        // problematic. For example, if a dtor uses another resources that is also a static global,
        // that resource could be destructed already, causing a crash. This gets around that by not
        // calling dtor's at all. The OS will release all memory from the process anyhow.
        _Exit(0);
    }

    ovrApp appState;
    ovrApp_Clear( &appState );
    appState.Java = java;

    //WaitForDebuggerToAttach3();

    if (immPlayerState.buildFlavorHeadless)
    {
        // Not sure if we actually need to do this but VrShell and Venues is...
        // Disable FBNS. We don't use it and it's a huge battery drain
        // If the API isn't found in Horizon, it gracefully falls back to no ConfigOptions
        ovrKeyValuePair options[1];
        options[0] = ovr_ConfigOption_CreateInternal(ovrConfigOption_DisableFbns, true);

        // Initialization call
        ovr_PlatformInitializeAndroidWithOptions(APP_ID, java.ActivityObject, java.Env, options, 1);
        ovr_User_GetAccessToken();

        #if !defined(DEBUG)
        ALOGV("ovrApp_PerformEntitlementCheck");
        ovr_Entitlement_GetIsViewerEntitled();
        #endif
    }

    ovrEgl_CreateContext( &appState.Egl, NULL );

//  enableDebugMessageCallback();

    EglInitExtensions();

    appState.UseMultiview = STEREO_MODE == ImmPlayer::StereoMode::Preferred && ( getGLExtensions().multi_view );

    ALOGV( "AppState UseMultiview : %d", appState.UseMultiview );

    appState.PerformanceParms = vrapi_DefaultPerformanceParms();
    appState.PerformanceParms.CpuLevel = CPU_LEVEL_MAX;
    appState.PerformanceParms.GpuLevel = GPU_LEVEL_MAX;
    appState.PerformanceParms.MainThreadTid = gettid();

#if MULTI_THREADED
    ovrRenderThread_Create( &appState.RenderThread, &appState.Java, &appState.Egl, appState.UseMultiview );
    // Also set the renderer thread to SCHED_FIFO.
    appState.PerformanceParms.RenderThreadTid = ovrRenderThread_GetTid( &appState.RenderThread );
#else
    ovrRenderer_Create( &appState.Renderer, &java, appState.UseMultiview );
#endif

    app->userData = &appState;
    app->onAppCmd = app_handle_cmd;

    while ( app->destroyRequested == 0 )
    {
        // Read all pending events.
        for ( ; ; )
        {
            int events;
            struct android_poll_source * source;
            const int timeoutMilliseconds = ( appState.Ovr == NULL && app->destroyRequested == 0 ) ? -1 : 0;
            if ( ALooper_pollAll( timeoutMilliseconds, NULL, &events, (void **)&source ) < 0 )
            {
                break;
            }

            // Process this event.
            if ( source != NULL )
            {
                source->process( app, source );
            }

            ovrApp_HandleVrModeChanges( &appState );
        }

        // TODO: make FFovR changes into a Message
        if (immPlayerState.pendingFoveationChange)
        {
            // Adopt requested foveation level for render strategy
            vrapi_SetPropertyInt(&appState.Java, VRAPI_FOVEATION_LEVEL, immPlayerState.foveationLevel);
            immPlayerState.pendingFoveationChange = false;
        }

        // Try again next frame if we can't acquire the lock.
        if (messageQueueLock.try_lock())
        {
            auto msg = messageQueue.begin();
            while (msg != messageQueue.end())
            {
                switch (msg->type)
                {
                    case ErrorDisconnected:
                    case ErrorUnknown:
                    case LoadImmPath:
                    {
                        handleMessageLoadImmPath(*msg);
                        msg = messageQueue.erase(msg);
                        break;
                    }
                    case UpdateMetadata:
                    {
                        if (immPlayerState.textTexture.metaStatus== AppImmPlayerState::TextTexture::Status::READY) {
#ifdef LOCALIZED_TEXT
                            char *title = piws2str(quillState.metadata.title);
                            char *artist = piws2str(quillState.metadata.artist);
                            char *date = piws2str(quillState.metadata.date);
                            char *description = piws2str(quillState.metadata.description);
                            updateTextTextureMetadata(&java, title, artist, date, description);
                            if (title != nullptr) free(title);
                            if (artist != nullptr) free(artist);
                            if (date != nullptr) free(date);
                            if (description != nullptr) free(description);
#endif
                            immPlayerState.textTexture.metaStatus = AppImmPlayerState::TextTexture::Status::TEXTURE_UPDATED;
                        }
                        ++msg;
                        // this msg will be processed again in renderViewpointsUI and erased there
                        break;
                    }
                    default:
                        ++msg;
                        break;
                }
            }
            messageQueueLock.unlock();
        }

        // TODO: make eye buffer changes into a Message
        // We received a request to scale the eye buffers after having created them prior to entering the render loop.
        if (appState.Renderer.FrameBuffer[0].Width != static_cast<int>(requestedEyeBufferWidth * immPlayerState.requestedEyeBufferScale))
        {
#if MULTI_THREADED == 0
            ovrRenderer_Destroy( &appState.Renderer );
            ovrRenderer_Create( &appState.Renderer, &java, appState.UseMultiview );
#endif
        }

        int docID = 0;

        // Shutdown requested by a back button process last frame. When unloading completes, we
        // clean-up state and redirect if necessary.
        if (shutdownRequested)
        {
            const bool isUnloaded = immPlayer.viewer == nullptr || !immPlayer.initialized
                    || immPlayer.viewer->GetDocumentLoadState(docID) == ImmPlayer::Player::LoadingState::Unloaded;

            if (!isUnloaded && (immPlayer.initialized && immPlayer.viewer->GetDocumentLoadState(docID) != ImmPlayer::Player::LoadingState::Unloading))
            {
                if (immPlayer.viewer->GetDocumentLoadState(docID) == ImmPlayer::Player::LoadingState::Loaded) {
                    /*registerEngagementEvent(
                        &appState.Java,
                        EngagementEvent::View,
                        immPlayer.viewer->GetViewTimeInMs(),
                        immPlayer.viewer->GetChapterViewCount());*/
                }

                immPlayer.viewer->Unload(docID);
            }

            if (!didProcessUnloadOnBackPress && isUnloaded)
            {
                ALOGV( " completed document unload clearing state..." );
                ALOGV( "     ovrApp_PushBlackFinal()" );
                ovrApp_PushBlackFinal( &appState );

                resetQuillPlayerState();

                // TODO: remove or rename this
                // This no longer deeplinks to tv but instead lets the OS hand focus back to the last activity/panelapp
                //deepLinkToTV(&appState.Java);
                // Finish the activity in order to cancel any in flight network requests
                ANativeActivity_finish(app->activity);

                didProcessUnloadOnBackPress = true; // Will be reset in activity onStart() for next document.
            }

            if (isUnloaded)
            {
                // Wait for destroy after finish.
                continue;
            }
        }

        ovrApp_HandleInput( &appState, app->activity );

        if ( appState.Ovr == NULL)
        {
            continue;
        }

        const bool isDownloadingDocument = immPlayerState.quillPath == "http";

        // Try to initialize the Quill player.
        if (immPlayer.viewer == nullptr)
        {
            // Quill path will be set to http while a download is pending on the Java side. When
            // the file has completed downloading and been copied to the data directory, the path
            // will be updated by QuillPlayerVrActivity.
            if (!isDownloadingDocument)
            {
                // For Oculus store build try to load:
                // 1) quill path in intent extra
                // 2) /sdcard/Oculus/quill/default.imm or /sdcard/Oculus/quill/default if present
                // 3) goro_the_beast embedded in APK

                const wchar_t *qPath = nullptr;
                bool shouldFree = false;

                if (immPlayerState.quillPath.empty())
                {
                    vrTrackingTransformLevel = VRAPI_TRACKING_TRANSFORM_SYSTEM_CENTER_EYE_LEVEL;

                    FILE *fp = fopen("/sdcard/Oculus/quill/default.imm", "rb");
                    if (fp)
                    {
                        fclose(fp);
                        qPath = L"/sdcard/Oculus/quill/default.imm";
                        ALOGV("    Loading Quill: default.imm from disk");
                    }
                    else if ((fp = fopen("/sdcard/Oculus/quill/default", "rb")))
                    {
                        fclose(fp);
                        qPath = L"/sdcard/Oculus/quill/default";
                        ALOGV("    Loading Quill: default authoring folder from disk");
                    }
                }
                else
                {
                    qPath = pistr2ws(getQuillPath());
                    ALOGV("    Loading quill from activity intent: %ls", qPath);
                    shouldFree = true;
                }

                initQuillPlayer(qPath, immPlayerState.renderingTechnique, &appState.Java);

                if (shouldFree)
                {
                    free((void *) qPath);
                }

                ALOGV("    Initialized Quill Viewer");
            }
            else
            {
                // Initialize Quill Player without loading a document path.
                // Note we can't access any of the document state APIs until Player::Load has been
                // called to initialize the document.
                initQuillPlayer(nullptr, immPlayerState.renderingTechnique, &appState.Java);
            }
        }

        // Use the Quill Viewer interface to render the scene if ready.
        if (immPlayer.viewer != nullptr)
        {
            // This is the only place the frame index is incremented, right before
            // calling vrapi_GetPredictedDisplayTime().
            appState.FrameIndex++;

            // Get the HMD pose, predicted for the middle of the time period during which
            // the new eye images will be displayed. The number of frames predicted ahead
            // depends on the pipeline depth of the engine and the synthesis rate.
            // The better the prediction, the less black will be pulled in at the edges.
            const double predictedDisplayTime = vrapi_GetPredictedDisplayTime( appState.Ovr, appState.FrameIndex );
            const ovrTracking2 tracking = vrapi_GetPredictedTracking2( appState.Ovr, predictedDisplayTime );

            appState.DisplayTime = predictedDisplayTime;

#if MULTI_THREADED
            // Render the eye images on a separate thread.
            ovrRenderThread_Submit( &appState.RenderThread, appState.Ovr,
                RENDER_FRAME, appState.FrameIndex, appState.DisplayTime, appState.SwapInterval,
                &tracking );
#else
            // If we are switching to another Quill we need to wait one frame for the CPU/GPU unload.
            // Once unloaded, load the new Quill path. Wait for http imm download to complete.
            bool isReadyToLoadNewDoc = true;
            if (immPlayer.initialized)
            {
                isReadyToLoadNewDoc = immPlayer.viewer->GetDocumentLoadState(docID) == ImmPlayer::Player::LoadingState::Unloaded;
            }

            if (!isDownloadingDocument && immPlayerState.loadNewDocument && isReadyToLoadNewDoc)
            {
                immPlayer.viewer->Deinit();
                // Reset the sound engine
                immPlayer.soundEngineBackend->Deinit();
                piSoundEngineBackend::Configuration config;
                config.mLowLatency =  true;  // enable android fast-path
                if (!immPlayer.soundEngineBackend->Init(nullptr, -1, &config))
                {
                    ALOGF("Can't init Audio360 soundEngineBackend");
                }
                wchar_t uiLibPath[1024];
                swprintf(uiLibPath, 1023, L"%suiLib", getAssetDirectory());

                ALOGV("previous document has unloaded... now loading new quill");
                const wchar_t * qPath = pistr2ws(getQuillPath());
                if (!loadQuillPath(qPath, immPlayerState.renderingTechnique))
                {
                    ALOGW("Can't init Quill Viewer %ls", qPath);
                }
                free((void *) qPath);
                immPlayerState.loadNewDocument = false;
            }

            ovrLayerProjection2 worldLayer;

            if (!isDownloadingDocument)
            {
                // When resuming an active Quill activity with no document loaded, if we've waited
                // enough frames without receiving a new document path we may want to show the error quill.
                bool mayShowErrorQuill = !shutdownRequested && framesSinceStart >= waitForDocFramesMax;

                if (!mayShowErrorQuill && !immPlayer.viewer->IsDocumentLoaded(docID))
                {
                    ++framesSinceStart;
                }

                // Show progress as soon as we start loading or downloading a quill.
                if (mayShowErrorQuill)
                {
                    if (!immPlayer.viewer->IsDocumentLoading(docID) && !immPlayer.viewer->IsDocumentLoaded(docID) && immPlayerState.quillPath.empty())
                    {
                        // We resumed the Quill Player activity without a start intent but the previous document
                        // was unloaded. This can happen if the app was started directly from the launcher icon.
                        ALOGV("  showing error quill due to empty quill path");
                    }
                    else if (immPlayer.viewer->DidDocumentLoadFail(docID))
                    {
                        ALOGV("  showing error quill because doc load failed");
                    }
                }

                if (immPlayer.viewer->IsDocumentLoaded(docID) && appState.PerformanceParms.CpuLevel != requestedCPULevel)
                {
                    // After loading completes, update to default rendering levels and allow dynamic clocking to ramp.
                    appState.PerformanceParms.CpuLevel = requestedCPULevel;
                    appState.PerformanceParms.GpuLevel = requestedGPULevel;
                    vrapi_SetClockLevels(appState.Ovr, appState.PerformanceParms.CpuLevel,
                                         appState.PerformanceParms.GpuLevel);
                    ALOGV("  loading completed... vrapi_SetClockLevels( %d, %d )",
                          appState.PerformanceParms.CpuLevel, appState.PerformanceParms.GpuLevel);
                }

                // Render eye images and setup the primary layer using ovrTracking2.
                worldLayer = ovrRenderer_RenderFrame(&appState.Renderer,
                                                     &appState.Java,
                                                     &tracking,
                                                     appState.Ovr,
                                                     appState.FrameIndex);
            }
            else
            {
                worldLayer = ovrRenderer_RenderDownloadProgress(&appState.Renderer,
                                                                &appState.Java,
                                                                &tracking,
                                                                appState.Ovr,
                                                                appState.FrameIndex);
            }

            // Don't submit the Quill frame unless the document is loaded or we're rendering the
            // progress component.
            if (immPlayer.viewer->IsDocumentLoading(docID) || isDownloadingDocument)
            {
                ovrLayerLoadingIcon2 loadingLayer = ovrRenderer_RenderTimewarpLoadingIcon(
                        appState.FrameIndex);

                if (!isDownloadingDocument)
                {
                    worldLayer = vrapi_DefaultLayerBlackProjection2();
                }

                const ovrLayerHeader2 * layers[] = { &worldLayer.Header, &loadingLayer.Header };
                ovrSubmitFrameDescription2 frameDesc = {};
                frameDesc.Flags = VRAPI_FRAME_FLAG_FLUSH;
                frameDesc.SwapInterval = appState.SwapInterval;
                frameDesc.FrameIndex = appState.FrameIndex;
                frameDesc.DisplayTime = appState.DisplayTime;
                frameDesc.LayerCount = 2;
                frameDesc.Layers = layers;

                vrapi_SubmitFrame2(appState.Ovr, &frameDesc);
            }
            else if (immPlayer.viewer->IsDocumentLoaded(docID))
            {
                const ovrLayerHeader2 * layers[] = { &worldLayer.Header };
                ovrSubmitFrameDescription2 frameDesc = {};
                frameDesc.Flags = 0;
                frameDesc.SwapInterval = appState.SwapInterval;
                frameDesc.FrameIndex = appState.FrameIndex;
                frameDesc.DisplayTime = appState.DisplayTime;
                frameDesc.LayerCount = 1;
                frameDesc.Layers = layers;

                // Hand over the eye images to the time warp.
                vrapi_SubmitFrame2(appState.Ovr, &frameDesc);
            }
            else
            {
                worldLayer = vrapi_DefaultLayerBlackProjection2();

                const ovrLayerHeader2 * layers[] = { &worldLayer.Header };
                ovrSubmitFrameDescription2 frameDesc = {};
                frameDesc.Flags = VRAPI_FRAME_FLAG_FLUSH;
                frameDesc.SwapInterval = appState.SwapInterval;
                frameDesc.FrameIndex = appState.FrameIndex;
                frameDesc.DisplayTime = appState.DisplayTime;
                frameDesc.LayerCount = 1;
                frameDesc.Layers = layers;

                vrapi_SubmitFrame2(appState.Ovr, &frameDesc);
            }
        }
#endif
        processOVRMessages(app, &appState.Java);
    }
    ALOGV("OVR app destroy requested");

    if (immPlayer.viewer != nullptr)
    {
        int docID = 0;
        immPlayer.viewer->CancelLoading(docID);
        while(immPlayer.viewer->IsDocumentLoading(docID))
        {
            ALOGV("Waiting for loading to stop");
            sleep(1);
        }
        immPlayer.viewer->UnloadAllSync();
    }

    destroyQuillPlayer();

#if MULTI_THREADED
    ovrRenderThread_Destroy( &appState.RenderThread );
#else
    ovrRenderer_Destroy( &appState.Renderer );
#endif

    ovrEgl_DestroyContext( &appState.Egl );


    ALOGV("OVR app requested exit");

    vrapi_Shutdown();

    (java.Vm)->DetachCurrentThread();
}

// endregion
