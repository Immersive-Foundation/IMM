using UnityEngine;
using UnityEngine.XR;
using UnityEditor;
using System.Collections.Generic;

//[ExecuteInEditMode]
public class ImmUnitySampleScript : MonoBehaviour
{
    public GameObject Viewpoint;
    public Object InputFile;

    // Scene management
    private int[] mDocumentID = new int[] { -1 };
    private ImmViewer.DocumentState tmpState0;
    private ImmViewer.LoadingState tmpStateCache0;
    private ImmViewer.DocumentState tmpState1;
    private ImmViewer.DocumentInfo tmpInfo0;
    private int mNumViewpoints = 0;
    private float mCameraHeight = 0.0f;
    private bool mFollowViewpoint = true;
    private int mCurrentSpawnArea = -1;
    public void Awake()
    {
        ImmViewer.InitializeSingleton();
        tmpState0.loadingState = ImmViewer.LoadingState.UNLOADED;
        tmpStateCache0 = ImmViewer.LoadingState.UNLOADED;
        mCameraHeight = Viewpoint.transform.position.y;
    }

    public void Update()
    {
        // File loaded or loading
        if (mDocumentID[0] != -1)
        {
            //====================================================================//
            //************************** LOADING STATES **************************//
            //====================================================================//

            tmpStateCache0 = tmpState0.loadingState;
            ImmViewer.Instance.GetDocumentState(ref tmpState0, mDocumentID[0]);
            if (tmpState0.loadingState == ImmViewer.LoadingState.UNLOADED)
            {
                Debug.Log("UNLOADED COMPLETE doc[0]");
                mDocumentID[0] = -1;
            }
            else if (tmpState0.loadingState == ImmViewer.LoadingState.FAILED)
            {
                Debug.Log("FAILED loading doc[0]!");
                tmpState0.loadingState = ImmViewer.LoadingState.UNLOADED;
                tmpStateCache0 = ImmViewer.LoadingState.UNLOADED;
            }
            else if (tmpStateCache0 == ImmViewer.LoadingState.LOADING && tmpState0.loadingState == ImmViewer.LoadingState.LOADED)
            {
                // Opt 1: Bring IMM document to origin when done loading.
                ImmViewer.Instance.SetDocumentToWorld(mDocumentID[0], Matrix4x4.identity);

                // Opt 2: Jump to IMM home spawnArea when done loading.
                {
                    mNumViewpoints = ImmViewer.Instance.GetSpawnAreaCount(mDocumentID[0]);
                    Debug.Assert(mNumViewpoints > 0);
                    Debug.Log("Number of spawnAreas = " + mNumViewpoints);
                    ImmViewer.SpawnArea vp;
                    mCurrentSpawnArea = ImmViewer.Instance.GetActiveSpawnAreaId(mDocumentID[0]);
                    ImmViewer.Instance.GetSpawnAreaInfo(mDocumentID[0], mCurrentSpawnArea, out vp);
                    // set tracking mode based on spawnArea type.
                    {
                        List<XRInputSubsystem> subsystems = new List<XRInputSubsystem>();
                        SubsystemManager.GetInstances<XRInputSubsystem>(subsystems);
                        for (int i = 0; i < subsystems.Count; i++)
                        {
                            subsystems[i].TrySetTrackingOriginMode(vp.type == ImmViewer.SpawnArea.Type.EyeLevel ? TrackingOriginModeFlags.Device : TrackingOriginModeFlags.Floor);
                        }
                    }
                    // use spawnArea transform info to set camera offset game object
                    {
                        Vector3 position = new Vector3(vp.transform.positionX, vp.transform.positionY, -vp.transform.positionZ);
                        Quaternion orientation = new Quaternion(-vp.transform.rotationX, -vp.transform.rotationY, vp.transform.rotationZ, vp.transform.rotationW);
                        float scale = vp.transform.scale;
                        Vector3 rot = orientation.eulerAngles;
                        Viewpoint.transform.position = position;
                        Viewpoint.transform.rotation = orientation;
                        Viewpoint.transform.localScale = new Vector3(scale, scale, scale);
                    }
                }
            }


            //====================================================================//
            //**************************** SPAWNAREAS ****************************//
            //====================================================================//
            // follow animated spawnArea
            if (tmpState0.loadingState == ImmViewer.LoadingState.LOADED)
            {
                int currentVP = ImmViewer.Instance.GetActiveSpawnAreaId(mDocumentID[0]);
                ImmViewer.SpawnArea vp;
                ImmViewer.Instance.GetSpawnAreaInfo(mDocumentID[0], currentVP, out vp);
                if (mFollowViewpoint && vp.animated)
                {
                    Vector3 position = new Vector3(vp.transform.positionX, vp.transform.positionY, -vp.transform.positionZ);
                    Quaternion orientation = new Quaternion(-vp.transform.rotationX, -vp.transform.rotationY, vp.transform.rotationZ, vp.transform.rotationW);
                    float scale = vp.transform.scale;
                    Vector3 rot = orientation.eulerAngles;

                    Viewpoint.transform.position = position;
                    Viewpoint.transform.rotation = orientation;
                    Viewpoint.transform.localScale = new Vector3(scale, scale, scale);
                }
            }

            if (Input.GetKeyUp(KeyCode.F) && mNumViewpoints > 0)
            {
                mFollowViewpoint = !mFollowViewpoint;
            }

            // Current expose hotkey for the first 6 spawnAreas, you could have more.
            if (Input.GetKeyUp(KeyCode.Alpha1) || Input.GetKeyUp(KeyCode.Keypad1)) { JumpToViewpoint(0); }
            if (Input.GetKeyUp(KeyCode.Alpha2) || Input.GetKeyUp(KeyCode.Keypad2)) { JumpToViewpoint(1); }
            if (Input.GetKeyUp(KeyCode.Alpha3) || Input.GetKeyUp(KeyCode.Keypad3)) { JumpToViewpoint(2); }
            if (Input.GetKeyUp(KeyCode.Alpha4) || Input.GetKeyUp(KeyCode.Keypad4)) { JumpToViewpoint(3); }
            if (Input.GetKeyUp(KeyCode.Alpha5) || Input.GetKeyUp(KeyCode.Keypad5)) { JumpToViewpoint(4); }
            if (Input.GetKeyUp(KeyCode.Alpha6) || Input.GetKeyUp(KeyCode.Keypad6)) { JumpToViewpoint(5); }

            //====================================================================//
            //************************** PLAYBACK CONTROLS ***********************//
            //====================================================================//
            // Skip Backward
            if (Input.GetKeyUp(KeyCode.Z)) { ImmViewer.Instance.SkipBack(mDocumentID[0]); }
            // Skip Forward
            if (Input.GetKeyUp(KeyCode.X)) { ImmViewer.Instance.SkipForward(mDocumentID[0]); }
            // Continue
            if (Input.GetKeyUp(KeyCode.C)) { ImmViewer.Instance.Continue(mDocumentID[0]); }
            // Restart
            if (Input.GetKeyUp(KeyCode.R)) { ImmViewer.Instance.Restart(mDocumentID[0]); }
            // Go to time
            if (Input.GetKeyUp(KeyCode.J)) { ImmViewer.Instance.SetTime(mDocumentID[0], 32000000, 0); }
            // Play/ Pause
            if (Input.GetKeyUp(KeyCode.P))
            {
                if (tmpState0.playingState == ImmViewer.PlayingState.PAUSED || tmpState0.playingState == ImmViewer.PlayingState.PAUSED_AND_HIDDEN)
                {
                    ImmViewer.Instance.Resume(mDocumentID[0]);
                }
                else
                {
                    ImmViewer.Instance.Pause(mDocumentID[0]);
                }
            }
        }

        //====================================================================//
        //************************** LOAD / UNLOAD ***************************//
        //====================================================================//
        if (Input.GetKeyUp(KeyCode.M))
        {
            if (mDocumentID[0] == -1)
            {
#if UNITY_EDITOR
                string immFilePath = AssetDatabase.GetAssetPath(InputFile.GetInstanceID());
#else
                string immFilePath = Application.streamingAssetsPath + "/Sample.imm";
#endif
                mDocumentID[0] = ImmViewer.Instance.Load(immFilePath);
                Debug.Log("LOAD doc[0] = " + immFilePath);
            }
        }

        if (Input.GetKeyUp(KeyCode.N))
        {
            if (tmpState0.loadingState == ImmViewer.LoadingState.LOADED)
            {
                Debug.Log("UNLOAD doc[0]");
                ImmViewer.Instance.Unload(mDocumentID[0]);
            }
        }

    }
    // This is called once per camaera, before rendering

    private void JumpToViewpoint(int vpId)
    {
        if (vpId >= mNumViewpoints) return;
        mFollowViewpoint = true;
        ImmViewer.Instance.SetActiveSpawnAreaId(mDocumentID[0], vpId);
        ImmViewer.SpawnArea vp;
        ImmViewer.Instance.GetSpawnAreaInfo(mDocumentID[0], vpId, out vp);
        Debug.Log("Jumpting to spawnArea" + vpId);
        // set tracking mode
        {
            List<XRInputSubsystem> subsystems = new List<XRInputSubsystem>();
            SubsystemManager.GetInstances<XRInputSubsystem>(subsystems);
            for (int i = 0; i < subsystems.Count; i++)
            {
                subsystems[i].TrySetTrackingOriginMode(vp.type == ImmViewer.SpawnArea.Type.EyeLevel ? TrackingOriginModeFlags.Device : TrackingOriginModeFlags.Floor);
            }
        }

        // use the data below to move Unity's camera to the desired spawnArea
        Vector3 position = new Vector3(vp.transform.positionX, vp.transform.positionY, -vp.transform.positionZ);
        Quaternion orientation = new Quaternion(-vp.transform.rotationX, -vp.transform.rotationY, vp.transform.rotationZ, vp.transform.rotationW);
        float scale = vp.transform.scale;
        Vector3 rot = orientation.eulerAngles;

        Viewpoint.transform.position = position;
        Viewpoint.transform.rotation = orientation;
        Viewpoint.transform.localScale = new Vector3(scale, scale, scale);

    }

}
