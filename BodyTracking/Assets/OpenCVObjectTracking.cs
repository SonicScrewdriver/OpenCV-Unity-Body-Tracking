using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using UnityEngine;

public class OpenCVObjectTracking : MonoBehaviour
{
    // Define the functions which can be called from the .dll.
    internal static class OpenCVInterop
    {
        [DllImport("OpenCV")]
        internal static extern int Init(ref int outCameraWidth, ref int outCameraHeight);

        [DllImport("OpenCV")]
        internal static extern int Close();

        [DllImport("OpenCV")]
        internal static extern int SetScale(int downscale);

        [DllImport("OpenCV")]
        internal unsafe static extern int Track(CvRectangle* outTracking, int maxTrackingCount, ref int outTrackingsCount);
    }
    [StructLayout(LayoutKind.Sequential, Size = 12)]

    public struct CvRectangle
    {
        public int Width, Height, X, Y;
    }
   
    public static List<Vector2> NormalizedTrackingPositions { get; private set; }
    public static Vector2 CameraResolution;

    /// <summary>
    /// Downscale factor to speed up detection.
    /// </summary>
    [SerializeField] int DetectionDownScale = 1;

    private bool _ready;
    private int _maxTrackingCount = 5;
    private CvRectangle[] _tracking;

    // Start is called before the first frame update
    void Start()
    {
        int camWidth = 0, camHeight = 0;
        int result = OpenCVInterop.Init(ref camWidth, ref camHeight);
        if (result < 0)
        {
            if (result == -1)
            {
                Debug.LogWarningFormat("[{0}] Failed to find cascades definition.", GetType());
            }
            else if (result == -2)
            {
                Debug.LogWarningFormat("[{0}] Failed to open camera stream.", GetType());
            } else if (result == -3)
            {
                Debug.LogWarningFormat("[{0}] First Frame Empty.", GetType());
            } else if (result == -4)
            {
                Debug.LogWarningFormat("[{0}] No ROI.", GetType());
            }
            return;
        } 

        CameraResolution = new Vector2(camWidth, camHeight);
        _tracking = new CvRectangle[_maxTrackingCount];
        NormalizedTrackingPositions = new List<Vector2>();
        OpenCVInterop.SetScale(DetectionDownScale);
        _ready = true;
    }

    void OnApplicationQuit()
    {
        if (_ready)
        {
            OpenCVInterop.Close();
        }
    }

    // Update is called once per frame
    void Update()
    {
        if (!_ready)
        {
            return;
        }
        ObjectTracking();

    }
    void ObjectTracking()
    {
        int detectedTrackingCount = 0;
        unsafe
        {
            fixed (CvRectangle* outTracking = _tracking)
            {
                OpenCVInterop.Track(outTracking, _maxTrackingCount, ref detectedTrackingCount);
            }
        }

        NormalizedTrackingPositions.Clear();
        for (int i = 0; i < detectedTrackingCount; i++)
        {
            NormalizedTrackingPositions.Add(new Vector2((_tracking[i].X * DetectionDownScale) / CameraResolution.x, 1f - ((_tracking[i].Y * DetectionDownScale) / CameraResolution.y)));
        }
        Debug.Log(NormalizedTrackingPositions);
    }

}
