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

// Object Tracking Rectangles!
struct Rectangle {
	Rectangle(int width, int height, int x, int y) : Width(width), Height(height), X(x), Y(y) {}
	int Width, Height, X, Y;
};

CascadeClassifier _bodyCascade;
String _windowName = "Unity OpenCV Prototype #2";
VideoCapture _capture;
int _scale = 1;
Ptr<Tracker> tracker;
string trackerType;
Rect2d patientBox;
bool patientSet;
std::vector<Rect> Bodies;


extern "C" int __declspec(dllexport) __stdcall  Init(int& outCameraWidth, int& outCameraHeight)
{
	// Load LBP face cascade.
	if (!_bodyCascade.load("haarcascade_frontalface_default.xml"))
		return -1;

	// Open the stream.
	_capture.open(0);
	if (!_capture.isOpened())
		return -2;

	outCameraWidth = _capture.get(CAP_PROP_FRAME_WIDTH);
	outCameraHeight = _capture.get(CAP_PROP_FRAME_HEIGHT);


}
extern "C" void __declspec(dllexport) __stdcall  SetPatient(Rect2d patientBoxData)
{
	patientBox = Bodies[0];
	patientSet = true;
	Mat frame;

	_capture >> frame;
	bool ok = _capture.read(frame);

	// List of tracker types in OpenCV 3.4.1
	string trackerTypes[8] = { "BOOSTING", "MIL", "KCF", "TLD","MEDIANFLOW", "GOTURN", "MOSSE", "CSRT" };
	// vector <string> trackerTypes(types, std::end(types));

	// Create a tracker
	trackerType = trackerTypes[7];


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


	rectangle(frame, patientBox, Scalar(255, 0, 0), 2, 1);

	putText(frame, "Mindful Garden OpenCV Test w/ " + trackerType, Point(120, 40), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(255, 0, 137), 2);
	bool trackerInit = tracker->init(frame, patientBox);

}

extern "C" void __declspec(dllexport) __stdcall  RefindPatient() {
	Mat frame;
	// >> shifts right and adds either 0s, if value is an unsigned type, or extends the top bit (to preserve the sign) if its a signed type.
	_capture >> frame;
	if (frame.empty())
		return;

	std::vector<Rect> bodies;
	// Convert the frame to grayscale for cascade detection.
	Mat grayscaleFrame;
	cvtColor(frame, grayscaleFrame, COLOR_BGR2GRAY);
	Mat resizedGray;
	// Scale down for better performance.
	resize(grayscaleFrame, resizedGray, Size(frame.cols / _scale, frame.rows / _scale));
	equalizeHist(resizedGray, resizedGray);

	// Detect faces.
	_bodyCascade.detectMultiScale(resizedGray, bodies);
	int maxArea = 0;
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
	// Draw faces.
	for (size_t i = 0; i < bodies.size(); i++)
	{
		//rectangle(frame, bodies[i], Scalar(255, 0, 0), 2, 1);
		Rect2d tracked_position = bodies[i];

		// Set the Tracker coordinates
		int _x = int(tracked_position.x);
		int _y = int(tracked_position.y);
		int _w = int(tracked_position.width);
		int _h = int(tracked_position.height);

		// Get the Biggest Face
		if (_w * _h > maxArea) {
			x = _x;
			y = _y;
			w = _w;
			h = _h;
			maxArea = w * h;
		}

	}
	if (maxArea > 0) {
		patientBox = Rect2d(x, y, w, h);
		rectangle(frame, patientBox, Scalar(255, 0, 137), 3, 1);
		putText(frame, "Patient", Point(((patientBox.x + patientBox.width)), (patientBox.y + patientBox.height + 20)), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 0, 137), 2);
		tracker = TrackerCSRT::create();
		bool trackerInit = tracker->init(frame, patientBox);
	}
	Bodies = bodies;

	putText(frame, "Mindful Garden OpenCV Test w/ " + trackerType, Point(120, 40), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(255, 0, 137), 2);
	// Display debug output.
	cv::imshow("Output", frame);
}

// Expose the function for DLL
extern "C" void __declspec(dllexport) __stdcall  Track(Rectangle * outTracking, int maxOutTrackingCount, int& outDetectedTrackingCount)
{
	if (patientSet) {
		Mat frame;
		_capture >> frame;
		if (frame.empty()) {
			return;
		}

		// Update the tracking result
		bool ok = tracker->update(frame, patientBox);

		if (ok)
		{
			// Tracking success : Draw the tracked object
			rectangle(frame, patientBox, Scalar(255, 0, 137), 3, 1);
			putText(frame, "Patient", Point(((patientBox.x + patientBox.width)), (patientBox.y + patientBox.height + 20)), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 0, 137), 2);
			outTracking[0] = Rectangle(patientBox.width, patientBox.height, patientBox.x, patientBox.y);
		}
		else
		{
			// Tracking failure detected.
			RefindPatient();
		}

		// Display tracker type on frame
		putText(frame, "Mindful Garden OpenCV Test w/ " + trackerType, Point(120, 40), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(255, 0, 137), 2);

		// Display frame.
		cv::imshow("Output", frame);
	}

}

extern "C" void __declspec(dllexport) __stdcall  Close()
{
	_capture.release();
}

extern "C" void __declspec(dllexport) __stdcall SetScale(int scale)
{
	_scale = scale;
}

extern "C" void __declspec(dllexport) __stdcall Detect(Rectangle * outBodies, int maxOutBodiesCount, int& outDetectedBodiesCount)
{
	Mat frame;
	// >> shifts right and adds either 0s, if value is an unsigned type, or extends the top bit (to preserve the sign) if its a signed type.
	_capture >> frame;
	if (frame.empty())
		return;

	std::vector<Rect> bodies;
	// Convert the frame to grayscale for cascade detection.
	Mat grayscaleFrame;
	cvtColor(frame, grayscaleFrame, COLOR_BGR2GRAY);
	Mat resizedGray;
	// Scale down for better performance.
	resize(grayscaleFrame, resizedGray, Size(frame.cols / _scale, frame.rows / _scale));
	equalizeHist(resizedGray, resizedGray);

	// Detect faces.
	_bodyCascade.detectMultiScale(resizedGray, bodies);
	int maxArea = 0;
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
	// Draw faces.
	for (size_t i = 0; i < bodies.size(); i++)
	{
		//rectangle(frame, bodies[i], Scalar(255, 0, 0), 2, 1);
		Rect2d tracked_position = bodies[i];

		// Set the Tracker coordinates
		int _x = int(tracked_position.x);
		int _y = int(tracked_position.y);
		int _w = int(tracked_position.width);
		int _h = int(tracked_position.height);

		// Get the Biggest Face
		if (_w * _h > maxArea) {
			x = _x;
			y = _y;
			w = _w;
			h = _h;
			maxArea = w * h;
		}

		// Send to application.
		outBodies[i] = Rectangle(bodies[i].x, bodies[i].y, bodies[i].width, bodies[i].height);
		outDetectedBodiesCount++;

		if (outDetectedBodiesCount == maxOutBodiesCount)
			break;
	}
	if (maxArea > 0) {
		patientBox = Rect2d(x, y, w, h);
		rectangle(frame, patientBox, Scalar(255, 0, 137), 3, 1);
		
		putText(frame, "Patient", Point(((patientBox.x + patientBox.width)), (patientBox.y + patientBox.height + 20)), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 0, 137), 2);

		tracker = TrackerCSRT::create();
		bool trackerInit = tracker->init(frame, patientBox);
	}
	Bodies = bodies;
	putText(frame, "Mindful Garden OpenCV Test w/ " + trackerType, Point(120, 40), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(255, 0, 137), 2);
	// Display debug output.
	cv::imshow("Output", frame);
}