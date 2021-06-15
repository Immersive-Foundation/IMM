using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Assertions;
using UnityEngine.XR;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public class ImmViewer : MonoBehaviour
{
    #region Public API

    public enum LoadingState : int
    {
        UNLOADED = 0,
        LOADING = 1,
        LOADED = 2,
        UNLOADING = 3,
        FAILED = 4,
    }

    public enum PlayingState : int
    {
        PLAYING = 0,
        PAUSED = 1,
        PAUSED_AND_HIDDEN = 2,
        WAITING = 3,
        FINISHED = 4,
    }

    public enum DocumentType : int
    {
        STILL = 0,
        ANIMATED = 1,
        COMIC = 2
    };

    public struct DocumentState
    {
        public LoadingState loadingState;
        public PlayingState playingState;
    }

    public struct DocumentInfo
    {
        public DocumentType documentType;
        public UInt16 isGrabbable;
        public UInt16 hasSound;
    }

    public struct PlayerInfo
    {
        public struct BackgroundColor
        {
            public float red;
            public float green;
            public float blue;
        }
        public BackgroundColor BackgrundColor;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct SpawnArea
    {
        public enum Type : int
        {
            EyeLevel = 0,
            FloorLevel = 1,
        };

        public struct Transform
        {
            public float positionX;
            public float positionY;
            public float positionZ;
            public float rotationX;
            public float rotationY;
            public float rotationZ;
            public float rotationW;
            public float scale;
        }

        public struct Volume
        {
            public enum Type : int
            {
                Sphere = 0,
                Box = 1,
            };

            public struct SphereExtent
            {
                float r;
            };
            public struct BoxExtent
            {
                float x, y, z;
            };
            public struct Offset
            {
                float x, y, z;
            };

            public Type type;
            public SphereExtent sphereExtent;
            public BoxExtent boxExtent;
            public Offset offset;
        }

        public struct Screenshot
        {
            System.UInt32 format;
            System.UInt32 width;
            System.UInt32 height;
            IntPtr pData;
        }

        public IntPtr name; // C++: wchar_t*
        public int version;
        public Type type;
        public bool animated;
        public Volume shape;
        public Transform transform;
        public int locomotion;
        public Screenshot screenshot;
    }

    public struct Bound3
    {
        public float mMinX;
        public float mMaxX;
        public float mMinY;
        public float mMaxY;
        public float mMinZ;
        public float mMaxZ;
    };

    #endregion

    //------------------------------------------------------------

    #region Unity API

    static ImmViewer mInstance = null;

    public static ImmViewer Instance
    {
        get
        {
            Assert.AreNotEqual(mInstance, null);
            return mInstance;
        }
    }

    public static void InitializeSingleton()
    {
        if (mInstance != null)
            return;
        mInstance = new GameObject("Immersive Viewer").AddComponent<ImmViewer>();
        if (mInstance.Init())
            DontDestroyOnLoad(mInstance.gameObject);
        else
        {
            Destroy(mInstance.gameObject);
            mInstance = null;
        }
    }

    public void OnEnable() { }
    public void OnDisable() { }
    public void OnDestroy()
    {
        if (mInstance == null)
            return;
        mInstance.End();
        Destroy(mInstance.gameObject);
        mInstance = null;
    }

    public void LateUpdate()
    {
        if (mPluginState != PluginState.INITIALIZED) return;
        Native.GlobalWork(1);
    }
    #endregion

    //------------------------------------------------------------

    #region Internal data
    private enum PluginState
    {
        UNINITIALIZED,
        INITIALIZED,
        FAILED
    }

    private class PerCameraInfo
    {
        public readonly CommandBuffer mCommandBuffer = new CommandBuffer();
        public int mID = -1;
        public readonly float[] mMatrixModel = new float[16];
        public readonly float[] mMatrixHeadView = new float[16];
        public readonly float[] mMatrixHeadProj = new float[16];
        public readonly float[] mMatrixLEyeView = new float[16];
        public readonly float[] mMatrixLEyeProj = new float[16];
        public readonly float[] mMatrixREyeView = new float[16];
        public readonly float[] mMatrixREyeProj = new float[16];
    }

    private struct Native
    {
        [DllImport("ImmUnityPlugin")] public static extern int Init(int colorSpace, int antialiasing, string logFullPathFileName, string tmpFolderName);
        [DllImport("ImmUnityPlugin")] public static extern void End();
        [DllImport("ImmUnityPlugin")] public static extern void GlobalWork(int enabled);
        [DllImport("ImmUnityPlugin")] public static extern int LoadFromFile(string name);
        [DllImport("ImmUnityPlugin")] public static extern void Unload(int id);
        [DllImport("ImmUnityPlugin")] public static extern void Pause(int id);
        [DllImport("ImmUnityPlugin")] public static extern void Resume(int id);
        [DllImport("ImmUnityPlugin")] public static extern void Show(int id);
        [DllImport("ImmUnityPlugin")] public static extern void Hide(int id);
        [DllImport("ImmUnityPlugin")] public static extern void Continue(int id);
        [DllImport("ImmUnityPlugin")] public static extern void SkipForward(int id);
        [DllImport("ImmUnityPlugin")] public static extern void SkipBack(int id);
        [DllImport("ImmUnityPlugin")] public static extern void Restart(int id);
        [DllImport("ImmUnityPlugin")] public static extern int GetChapterCount(int id);
        [DllImport("ImmUnityPlugin")] public static extern int GetCurrentChapter(int id);
        [DllImport("ImmUnityPlugin")] public static extern void SetTime(int id, long timeSinceStart, long timeSinceStop);
        [DllImport("ImmUnityPlugin")] public static extern void GetTime(int id, ref long timeSinceStart, ref long timeSinceStop);
        [DllImport("ImmUnityPlugin")] public static extern void SetDocumentToWorld(int id, float[] modelMatrix);
        [DllImport("ImmUnityPlugin")] public static extern float GetSound(int id);
        [DllImport("ImmUnityPlugin")] public static extern float SetSound(int id, float volume);
        [DllImport("ImmUnityPlugin")] public static extern void GetPlayerInfo(ref PlayerInfo info);
        [DllImport("ImmUnityPlugin")] public static extern void GetDocumentInfo(ref DocumentInfo info, int id);
        [DllImport("ImmUnityPlugin")] public static extern void GetDocumentState(ref DocumentState state, int id);
        [DllImport("ImmUnityPlugin")] public static extern int GetSpawnAreaCount(int docId);
        [DllImport("ImmUnityPlugin")] public static extern void GetSpawnAreaInfo(int docId, int spawnAreaId, out SpawnArea viewpoint);
        [DllImport("ImmUnityPlugin")] public static extern int GetActiveSpawnAreaId(int docId);
        [DllImport("ImmUnityPlugin")] public static extern void SetActiveSpawnAreaId(int docId, int activeSpawnAreaId);
        [DllImport("ImmUnityPlugin")] public static extern void GetBoundingBox(int id, ref Bound3 bound);
        [DllImport("ImmUnityPlugin")] public static extern IntPtr GetRenderEventFunc();
        [DllImport("ImmUnityPlugin")] public static extern void SetMatrices(int cameraId, int stereoMode, float[] worldToHead, float[] prjHead, float[] worldToLEye, float[] prjLeft, float[] worldToREye, float[] prjRight);

    }

    private readonly Dictionary<Camera, PerCameraInfo> m_Cameras = new Dictionary<Camera, PerCameraInfo>();
    private ImmViewer.PlayerInfo mPlayerInfo;
    private PluginState mPluginState = PluginState.UNINITIALIZED;
    private readonly float[] mTempForConversions = new float[16];
    #endregion

    //------------------------------------------------------------

    #region Internal functions
    private void ConvertMatrixToArray(float[] dst, Matrix4x4 matrix)
    {
        for (int i = 0; i < 16; i++) { dst[i] = matrix[i]; }
    }

    private void Cleanup()
    {
        foreach (var cam in m_Cameras)
        {
            if (cam.Key)
            {
                PerCameraInfo info = cam.Value;
                cam.Key.RemoveCommandBuffer(CameraEvent.AfterImageEffectsOpaque, info.mCommandBuffer);
            }
        }
        m_Cameras.Clear();
    }

    private void MyPreCull(Camera cam)
    {
        if (!cam) return;
        //Camera.MonoOrStereoscopicEye activeEye = cam.stereoActiveEye;
        bool stereoEnabled = cam.stereoEnabled;
        //Debug.Log("Camera: " + cam.name + "  " + cam.stereoEnabled + "  " + kk++);
#if UNITY_EDITOR
        int pluginEventType = 0;
        int stereoMode = 0;

             if (stereoEnabled == false) { stereoMode = 0; }
        else if (stereoEnabled == true && XRSettings.stereoRenderingMode == XRSettings.StereoRenderingMode.MultiPass ) { stereoMode = 1; }
        else if (stereoEnabled == true && XRSettings.stereoRenderingMode == XRSettings.StereoRenderingMode.SinglePass) { stereoMode = 2; }
        else if (stereoEnabled == true && XRSettings.stereoRenderingMode == XRSettings.StereoRenderingMode.SinglePassInstanced) { stereoMode = 2; } // TODO: This is not working correctly.
#else
        int pluginEventType = 0;
        int stereoMode = stereoEnabled ? 2 : 0;
#endif

        // add command buffer to camera, if needed
        if (!m_Cameras.ContainsKey(cam))
        {

            PerCameraInfo info = new PerCameraInfo();
            info.mID = m_Cameras.Count;
            // set IMM rendering work
            info.mCommandBuffer.name = "Render IMM Content";
            info.mCommandBuffer.IssuePluginEvent(Native.GetRenderEventFunc(), pluginEventType + (info.mID << 8));

            m_Cameras[cam] = info;
            cam.AddCommandBuffer(CameraEvent.AfterImageEffectsOpaque, info.mCommandBuffer);
            Debug.Log("Added Command Buffer for camera \"" + cam.name + "\", " + ((cam.stereoEnabled) ? "Stereo" : "Mono" + " ID = " + info.mID));
        }


        {
            if (mInstance == null)
                return;
            mInstance.GetPlayerInfo(ref mPlayerInfo);
            Color col = new Color(mPlayerInfo.BackgrundColor.red, mPlayerInfo.BackgrundColor.green, mPlayerInfo.BackgrundColor.blue);

            // bug in unity -> camera background color does not respect ColorSpace
            if (QualitySettings.activeColorSpace == ColorSpace.Linear)
                col = col.gamma;
            cam.clearFlags = CameraClearFlags.SolidColor;
            cam.backgroundColor = col;
            PerCameraInfo info = m_Cameras[cam];

            ConvertMatrixToArray(info.mMatrixHeadView, cam.worldToCameraMatrix);
            ConvertMatrixToArray(info.mMatrixLEyeView, cam.GetStereoViewMatrix(Camera.StereoscopicEye.Left));
            ConvertMatrixToArray(info.mMatrixREyeView, cam.GetStereoViewMatrix(Camera.StereoscopicEye.Right));
            ConvertMatrixToArray(info.mMatrixHeadProj, GL.GetGPUProjectionMatrix(cam.projectionMatrix, true));
            ConvertMatrixToArray(info.mMatrixLEyeProj, GL.GetGPUProjectionMatrix(cam.GetStereoProjectionMatrix(Camera.StereoscopicEye.Left), true));
            ConvertMatrixToArray(info.mMatrixREyeProj, GL.GetGPUProjectionMatrix(cam.GetStereoProjectionMatrix(Camera.StereoscopicEye.Right), true));

            Native.SetMatrices(info.mID, stereoMode, info.mMatrixHeadView, info.mMatrixHeadProj,
                                               info.mMatrixLEyeView, info.mMatrixLEyeProj,
                                               info.mMatrixREyeView, info.mMatrixREyeProj);
        }
    }

    private bool Init()
    {
        if (mPluginState == PluginState.INITIALIZED)
            return true;

        ColorSpace configColorSpace = QualitySettings.activeColorSpace;
        int configAA = QualitySettings.antiAliasing;
        bool configVR = XRSettings.enabled;
        XRSettings.StereoRenderingMode configStereoMode = XRSettings.stereoRenderingMode;

        Debug.Log("Start : AA=" + configAA + ", VR=" + (configVR ? "Yes" : "No") +
            ", ColorSpace=" + configColorSpace.ToString() + ", StereoPath=" +
            configStereoMode.ToString() + ", tmpFolder = " + Application.temporaryCachePath);

        if (Native.Init((configColorSpace == ColorSpace.Linear) ? 0 : 1,
            configAA, "unity_imm_debug.txt", Application.temporaryCachePath + "/tmp") < 0)
        {
            mPluginState = PluginState.FAILED;
            Debug.Log("ImmViewer::Init FAILED");
            return false;
        }

        Cleanup();
        Camera.onPreCull += MyPreCull;

        mPluginState = PluginState.INITIALIZED;
        return true;
    }

    private void End()
    {
        if (mPluginState != PluginState.INITIALIZED)
            return;
        Cleanup();
        Camera.onPreCull -= MyPreCull;
        Native.End();
        mPluginState = PluginState.UNINITIALIZED;
    }

#endregion

    //------------------------------------------------------------

#region Public API
    public int   Load(string name) { if (mPluginState != PluginState.INITIALIZED) return -1; return Native.LoadFromFile(name); }
    public void  Unload(int id) { if (mPluginState != PluginState.INITIALIZED) return; Native.Unload(id); }
    public void  Pause(int id) { if (mPluginState != PluginState.INITIALIZED) return; Native.Pause(id); }
    public void  Resume(int id) { if (mPluginState != PluginState.INITIALIZED) return; Native.Resume(id); }
    public void  Show(int id) { if (mPluginState != PluginState.INITIALIZED) return; Native.Show(id); }
    public void  Hide(int id) { if (mPluginState != PluginState.INITIALIZED) return; Native.Hide(id); }
    public void  Continue(int id) { if (mPluginState != PluginState.INITIALIZED) return; Native.Continue(id); }
    public void  SkipBack(int id) { if (mPluginState != PluginState.INITIALIZED) return; Native.SkipBack(id); }
    public void  SkipForward(int id) { if (mPluginState != PluginState.INITIALIZED) return; Native.SkipForward(id); }
    public void  Restart(int id) { if (mPluginState != PluginState.INITIALIZED) return; Native.Restart(id); }
    public int   GetChapterCount(int id) { if (mPluginState != PluginState.INITIALIZED) return 0; return Native.GetChapterCount(id); }
    public int   GetCurrentChapter(int id) { if (mPluginState != PluginState.INITIALIZED) return 0; return Native.GetCurrentChapter(id); }
    public void  SetTime(int id, long timeSinceStart, long timeSinceStop) { Native.SetTime(id, timeSinceStart, timeSinceStop); }
    public void  GetTime(int id, ref long timeSinceStart, ref long timeSinceStop) { Native.GetTime(id, ref timeSinceStart, ref timeSinceStop); }
    public float GetSound(int id) { return Native.GetSound(id); }
    public void  SetSound(int id, float volume) { Native.SetSound(id, volume); }
    public void  GetPlayerInfo(ref PlayerInfo info) { if (mPluginState != PluginState.INITIALIZED) return; Native.GetPlayerInfo(ref info); }
    public void  GetDocumentInfo(ref DocumentInfo info, int id) { if (mPluginState != PluginState.INITIALIZED) return; Native.GetDocumentInfo(ref info, id); }
    public void  GetDocumentState(ref DocumentState state, int id) { if (mPluginState != PluginState.INITIALIZED) return; Native.GetDocumentState(ref state, id); }
    public int GetSpawnAreaCount(int id) { if (mPluginState != PluginState.INITIALIZED) return 0; return Native.GetSpawnAreaCount(id); }
    public void GetSpawnAreaInfo(int id, int spawnAreaId, out SpawnArea viewpoint) { Native.GetSpawnAreaInfo(id, spawnAreaId, out viewpoint); }
    public void  SetDocumentToWorld(int id, Matrix4x4 documentToWorld) { if (mPluginState != PluginState.INITIALIZED) return; ConvertMatrixToArray(mTempForConversions, documentToWorld); Native.SetDocumentToWorld(id, mTempForConversions); }
    public int GetActiveSpawnAreaId(int docId) { return Native.GetActiveSpawnAreaId(docId); }
    public void SetActiveSpawnAreaId(int docId, int activeSpawnAreaId) { Native.SetActiveSpawnAreaId(docId, activeSpawnAreaId); }


#endregion
}
