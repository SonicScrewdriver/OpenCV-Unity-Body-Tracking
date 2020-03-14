using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

public class OpenCVBodyTracking : MonoBehaviour
{
    // Define the functions which can be called from the .dll.
    internal static class OpenCVInterop
    {
        [DllImport("OpenCV", CharSet = CharSet.Auto, CallingConvention = CallingConvention.StdCall)]
        internal unsafe static extern int Init(ref int outCameraWidth, ref int outCameraHeight, ref int tracker, ref int cascade);

        [DllImport("OpenCV")]
        internal static extern int Close();

        [DllImport("OpenCV")]
        internal static extern int SetScale(int downscale);

        [DllImport("OpenCV")]
        internal unsafe static extern void Detect(CvRectangle* outBodies, int maxOutBodiesCount, ref int outDetectedBodiesCount);

        [DllImport("OpenCV")]
        internal unsafe static extern int Track(CvRectangle* outTracking, int maxTrackingCount, ref int outTrackingsCount);
    }

    [StructLayout(LayoutKind.Sequential, Size = 12)]

    // Set the struct used to retrieve the box data from OpenCV
    public struct CvRectangle
    {
        public int Width, Height, X, Y;
    }

    // Set variables and parameters
    public static List<Vector2> NormalizedTrackingPositions { get; private set; }
    public static List<Vector2> NormalizedBodyPositions { get; private set; }
    public static Vector2 CameraResolution;

    public enum TrackerTypes { BOOSTING = 0, MIL = 1, KCF = 2, TLD = 3, MEDIANFLOW = 4, GOTURN = 5, MOSSE = 6, CSRT = 7 }
    public enum CascadeTypes { HAAR_FACE = 0, HAAR_BODY = 1, HAAR_UPPERBODY = 2, LBP_FACE = 3}
    public TrackerTypes trackerTypes = TrackerTypes.CSRT;
    public CascadeTypes cascadeTypes = CascadeTypes.HAAR_FACE;
    /// Fields available in the inspector
    [SerializeField] private int DetectionDownScale = 1;
    [SerializeField] private int _maxTrackCount = 5;

    private bool _ready;
    private CvRectangle[] _tracking;
    private CvRectangle[] _bodies;
    private CvRectangle patientBody;
    private int frameRate = 0;

    // Start is called before the first frame update
    private void Start()
    {
     
        initOpenCV();
    }

    private void initOpenCV()
    {
        int camWidth = 0, camHeight = 0;
        int tracker = (int)trackerTypes;
        int cascade = (int)cascadeTypes;

  
        int result = OpenCVInterop.Init(ref camWidth, ref camHeight, ref tracker, ref cascade);   
        // Run the OpenV Init script, and log the result

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

        // Prepare all variables and arrays for OpenCV
        CameraResolution = new Vector2(camWidth, camHeight);
        _bodies = new CvRectangle[_maxTrackCount];
        _tracking = new CvRectangle[_maxTrackCount];
        patientBody = new CvRectangle();
        NormalizedBodyPositions = new List<Vector2>();
        NormalizedTrackingPositions = new List<Vector2>();
        OpenCVInterop.SetScale(DetectionDownScale);
        DetectBodies();
        frameRate = 0; 
        _ready = true; 
    }

    private void OnApplicationQuit()
    {
        if (_ready)
        {
            OpenCVInterop.Close();
        }
    }

    // Update is called once per frame
    private void Update()
    {
        // Wait for init
        if (!_ready)
        {
            return;
        }
        else
        {
            frameRate++;
            // Detect the bodies every 240 frames for efficiency and accuracy
            if (frameRate % 240 == 0)
            {
                DetectBodies();
            }
            // Otherwise continue tracking the existing object
            else
            {
                PatientTracking();
            }
        } 
    }

    private void PatientTracking()
    {
        // Unsafe codeblock to retrieve data from OpenCV 
        int detectedTrackingCount = 0;
        unsafe
        {
            fixed (CvRectangle* outTracking = _tracking)
            {
                OpenCVInterop.Track(outTracking, _maxTrackCount, ref detectedTrackingCount);
            }
        }

        // Record the Normalized Tracking Positions
        NormalizedTrackingPositions.Clear();
        for (int i = 0; i < detectedTrackingCount; i++)
        {
            NormalizedTrackingPositions.Add(new Vector2((_tracking[i].X * DetectionDownScale) / CameraResolution.x, 1f - ((_tracking[i].Y * DetectionDownScale) / CameraResolution.y)));
        }

        patientBody = _tracking[0];
        // Log current patient position for debugging
        Debug.Log("Patient At: (x=" + _tracking[0].X + " y=" + _tracking[0].Y + " width=" + _tracking[0].Width + " height=" + _tracking[0].Height + ")");
    }

    private void DetectBodies()
    {
        // Unsafe codeblock to retrieve data from OpenCV 
        int detectedBodyCount = 0;
        unsafe
        {
            fixed (CvRectangle* outBodies = _bodies)
            {
                OpenCVInterop.Detect(outBodies, _maxTrackCount, ref detectedBodyCount);
            }
        }

        // Record the Normalized Tracking Positions
        NormalizedBodyPositions.Clear();
        for (int i = 0; i < detectedBodyCount; i++)
        {
            NormalizedBodyPositions.Add(new Vector2((_bodies[i].X * DetectionDownScale) / CameraResolution.x, 1f - ((_bodies[i].Y * DetectionDownScale) / CameraResolution.y)));
        }
        patientBody = _tracking[0];
    }
}