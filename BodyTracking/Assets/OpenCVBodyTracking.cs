using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using UnityEngine;


public class OpenCVBodyTracking : MonoBehaviour
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
        internal static extern int SetPatient(CvRectangle patientBody);

        [DllImport("OpenCV")]
        internal unsafe static extern void Detect(CvRectangle* outBodies, int maxOutBodiesCount, ref int outDetectedBodiesCount);


        [DllImport("OpenCV")]
        internal unsafe static extern int Track(CvRectangle* outTracking, int maxTrackingCount, ref int outTrackingsCount);
    }
    [StructLayout(LayoutKind.Sequential, Size = 12)]

    public struct CvRectangle
    {
        public int Width, Height, X, Y;
    }

    public static List<Vector2> NormalizedTrackingPositions { get; private set; }
    public static List<Vector2> NormalizedBodyPositions { get; private set; }
    public static Vector2 CameraResolution;

    /// Downscale factor to speed up detection.
    [SerializeField] int DetectionDownScale = 1;
    [SerializeField] private int _maxTrackCount = 5;

    private bool _ready;
    private CvRectangle[] _tracking;
    private CvRectangle[] _bodies;
    private CvRectangle patientBody;

    // Start is called before the first frame update
    void Start()
    {
        initOpenCV();
    }

    void initOpenCV()
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
            }
            else if (result == -3)
            {
                Debug.LogWarningFormat("[{0}] No Bodies Detected.", GetType());
            }
            else if (result == -4)
            {
                Debug.LogWarningFormat("[{0}] Tracking Error.", GetType());
            }
            return;
        }

        CameraResolution = new Vector2(camWidth, camHeight);
        _bodies = new CvRectangle[_maxTrackCount];
        _tracking = new CvRectangle[_maxTrackCount];
        patientBody = new CvRectangle();
        NormalizedBodyPositions = new List<Vector2>();
        NormalizedTrackingPositions = new List<Vector2>();
        OpenCVInterop.SetScale(DetectionDownScale);
        DetectBodies();
        _ready = SetPatient();
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
        PatientTracking();

    }
    void PatientTracking()
    {
        int detectedTrackingCount = 0;
        unsafe
        {
            fixed (CvRectangle* outTracking = _tracking)
            {
                OpenCVInterop.Track(outTracking, _maxTrackCount, ref detectedTrackingCount);
            }
        }

        NormalizedTrackingPositions.Clear();
        for (int i = 0; i < detectedTrackingCount; i++)
        {
            NormalizedTrackingPositions.Add(new Vector2((_tracking[i].X * DetectionDownScale) / CameraResolution.x, 1f - ((_tracking[i].Y * DetectionDownScale) / CameraResolution.y)));
        }
        Debug.Log("Patient At:" + _tracking[0].X + _tracking[0].Y + _tracking[0].Width + _tracking[0].Height);
    }

    void DetectBodies()
    {
        int detectedBodyCount = 0;
        unsafe
        {
            fixed (CvRectangle* outBodies = _bodies)
            {
                OpenCVInterop.Detect(outBodies, _maxTrackCount, ref detectedBodyCount);
            }
        }

        NormalizedBodyPositions.Clear();
        for (int i = 0; i < detectedBodyCount; i++)
        {
            NormalizedBodyPositions.Add(new Vector2((_bodies[i].X * DetectionDownScale) / CameraResolution.x, 1f - ((_bodies[i].Y * DetectionDownScale) / CameraResolution.y)));
        }
    }
    bool SetPatient()
    {
        // Set the Patient's body as the first body detected
        patientBody = _bodies[0];

        // If the Patient Body information is not null/unset, the initialization is ready. 
        if (patientBody.X != 0)
        {
            unsafe
            {
                    OpenCVInterop.SetPatient(patientBody);               
            }
            return true;
        } else
        {
            return false;
        }
    }
}

