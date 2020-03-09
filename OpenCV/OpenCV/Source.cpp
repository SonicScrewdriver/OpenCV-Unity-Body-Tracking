#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <opencv2/core/utility.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <cstring>
#include <ctime>

using namespace std;
using namespace cv;
// OBJECT TRACKING PROTOTYPE

// Convert to string
#define SSTR( x ) static_cast< std::ostringstream & >( \
( std::ostringstream() << std::dec << x ) ).str()

// Declare structure to be used to pass data from C++ to Mono. FACES
struct Circle
{
	Circle(int x, int y, int radius) : X(x), Y(y), Radius(radius) {}
	int X, Y, Radius;
};

// Object Tracking Rectangles!
struct Rectangle {
	Rectangle(int width, int height, int x, int y) : Width(width), Height(height), X(x), Y(y) {}
	int Width, Height, X, Y;
};

CascadeClassifier _faceCascade;
String _windowName = "Unity OpenCV Prototype #2";
VideoCapture _capture;
int _scale = 1;
Ptr<Tracker> tracker;
Rect2d bbox;
string trackerType;


extern "C" int __declspec(dllexport) __stdcall  Init(int& outCameraWidth, int& outCameraHeight)
{
	// Load LBP face cascade.
	if (!_faceCascade.load("lbpcascade_frontalface_improved.xml"))
		return -1;

	// Open the stream.
	_capture.open(0);
	if (!_capture.isOpened())
		return -2;

	outCameraWidth = _capture.get(CAP_PROP_FRAME_WIDTH);
	outCameraHeight = _capture.get(CAP_PROP_FRAME_HEIGHT);

	Mat frame;

	_capture >> frame;
	bool ok = _capture.read(frame);
	if (frame.empty()) {
		return -3;
	}

	// List of tracker types in OpenCV 3.4.1
	string trackerTypes[8] = { "BOOSTING", "MIL", "KCF", "TLD","MEDIANFLOW", "GOTURN", "MOSSE", "CSRT" };
	// vector <string> trackerTypes(types, std::end(types));

	// Create a tracker
	trackerType = trackerTypes[2];


	if (trackerType == "BOOSTING")
		tracker = TrackerBoosting::create();
	if (trackerType == "MIL")
		tracker = TrackerMIL::create();
	if (trackerType == "KCF")
		tracker = TrackerKCF::create();
	if (trackerType == "TLD")
		tracker = TrackerTLD::create();
	if (trackerType == "MEDIANFLOW")
		tracker = TrackerMedianFlow::create();
	if (trackerType == "GOTURN")
		tracker = TrackerGOTURN::create();
	if (trackerType == "MOSSE")
		tracker = TrackerMOSSE::create();
	if (trackerType == "CSRT")
		tracker = TrackerCSRT::create();


	// Define initial bounding box 
	Rect2d bbox(180, 70, 200, 200);
	rectangle(frame, bbox, Scalar(255, 0, 0), 2, 1);

	imshow("Tracking", frame);
	bool trackerInit = tracker->init(frame, bbox);

	if (trackerInit) {
		return 1;
	}
}



// Expose the function for DLL
extern "C" void __declspec(dllexport) __stdcall  Track(Rectangle * outTracking, int maxOutTrackingCount, int& outDetectedTrackingCount)
{
	Mat frame;
	_capture >> frame;
	if (frame.empty()) {
		return;
	}

	// Update the tracking result
	bool ok = tracker->update(frame, bbox);


	if (ok)
	{
		// Tracking success : Draw the tracked object
		rectangle(frame, bbox, Scalar(255, 0, 0), 2, 1);
		outTracking[0] = Rectangle(bbox.width, bbox.height, bbox.x, bbox.y);
	}
	else
	{
		// Tracking failure detected.
		putText(frame, "Tracking failure detected", Point(100, 80), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0, 0, 255), 2);
	}

	// Display tracker type on frame
	putText(frame, trackerType + " Tracker", Point(100, 20), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50, 170, 50), 2);

	// Display frame.
	imshow("Tracking", frame);

}

extern "C" void __declspec(dllexport) __stdcall  Close()
{
	_capture.release();
}

extern "C" void __declspec(dllexport) __stdcall SetScale(int scale)
{
	_scale = scale;
}

extern "C" void __declspec(dllexport) __stdcall Detect(Circle * outFaces, int maxOutFacesCount, int& outDetectedFacesCount)
{
	Mat frame;
	// >> shifts right and adds either 0s, if value is an unsigned type, or extends the top bit (to preserve the sign) if its a signed type.
	_capture >> frame;
	if (frame.empty())
		return;

	std::vector<Rect> faces;
	// Convert the frame to grayscale for cascade detection.
	Mat grayscaleFrame;
	cvtColor(frame, grayscaleFrame, COLOR_BGR2GRAY);
	Mat resizedGray;
	// Scale down for better performance.
	resize(grayscaleFrame, resizedGray, Size(frame.cols / _scale, frame.rows / _scale));
	equalizeHist(resizedGray, resizedGray);

	// Detect faces.
	_faceCascade.detectMultiScale(resizedGray, faces);

	// Draw faces.
	for (size_t i = 0; i < faces.size(); i++)
	{
		Point center(_scale * (faces[i].x + faces[i].width / 2), _scale * (faces[i].y + faces[i].height / 2));
		ellipse(frame, center, Size(_scale * faces[i].width / 2, _scale * faces[i].height / 2), 0, 0, 360, Scalar(0, 0, 255), 4, 8, 0);

		// Send to application.
		outFaces[i] = Circle(faces[i].x, faces[i].y, faces[i].width / 2);
		outDetectedFacesCount++;

		if (outDetectedFacesCount == maxOutFacesCount)
			break;
	}

	// Display debug output.
	imshow("tracker", frame);
}