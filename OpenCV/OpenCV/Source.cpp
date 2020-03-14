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

// Namespaces
using namespace std;
using namespace cv;

// Object Tracking Rectangle struct used to send data back to Unity
struct Rectangle {
	Rectangle(int width, int height, int x, int y) : Width(width), Height(height), X(x), Y(y) {}
	int Width, Height, X, Y;
};

// Key parameters / variables.
CascadeClassifier _objectCascade;
String _windowName = "Mindful Garden OpenCV Test w/ ";
VideoCapture _capture;
int _scale = 1;
Ptr<Tracker> tracker;
string trackerType;
Rect2d patientBox;
bool patientSet;
std::vector<Rect> Objects;
int maxObjects = 0;


extern "C" int __declspec(dllexport) __stdcall  Init(int& outCameraWidth, int& outCameraHeight)
{
	// Load the chosen cascade file
	if (!_objectCascade.load("haarcascade_frontalface_default.xml"))
		return -1;

	// Open the Webcam stream.
	// This can be done in Unity, but we will need to modify the frame to return BGR color 
	// https://answers.opencv.org/question/202312/create-dll-using-trackerkcf-for-a-unity-project-crash-when-updating-the-tracker-with-the-dll
	_capture.open(0);
	if (!_capture.isOpened())
		return -2;

	// Set the Camera Width and Height
	outCameraWidth = _capture.get(CAP_PROP_FRAME_WIDTH);
	outCameraHeight = _capture.get(CAP_PROP_FRAME_HEIGHT);


}
// Set the Scale used for the calculations (Set in Unity) 
extern "C" void __declspec(dllexport) __stdcall SetScale(int scale)
{
	_scale = scale;
}

// Detect the chosen object - to be run once upon init (Could be made more efficient later). 
extern "C" void __declspec(dllexport) __stdcall Detect(Rectangle * outObjects, int maxOutObjectsCount, int& outDetectedObjectsCount)
{
	// Set the global variable to reuse during refind loops
	maxObjects = maxOutObjectsCount;

	// Grab the current frame from the webcam, and return if it's empty. 
	Mat frame;
	_capture >> frame;
	if (frame.empty())
		return;

	// Create the array we will use to store the detected objects
	std::vector<Rect> objects;

	// Convert the frame to grayscale for cascade detection.
	Mat grayscaleFrame;
	cvtColor(frame, grayscaleFrame, COLOR_BGR2GRAY);
	Mat resizedGray;

	// Scale down for better performance.
	resize(grayscaleFrame, resizedGray, Size(frame.cols / _scale, frame.rows / _scale));
	equalizeHist(resizedGray, resizedGray);

	// Detect faces.
	_objectCascade.detectMultiScale(resizedGray, objects);
	int maxArea = 0;
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;

	// Determine which object is the largest
	for (size_t i = 0; i < objects.size(); i++)
	{
		//rectangle(frame, objects[i], Scalar(255, 0, 0), 2, 1);
		Rect2d tracked_position = objects[i];

		// Set the Tracker coordinates
		int _x = int(tracked_position.x);
		int _y = int(tracked_position.y);
		int _w = int(tracked_position.width);
		int _h = int(tracked_position.height);

		// Comparing the max areas of each box
		if (_w * _h > maxArea) {
			x = _x;
			y = _y;
			w = _w;
			h = _h;
			maxArea = w * h;
		}

		// Set the current object for transferral to Unity.
		outObjects[i] = Rectangle(objects[i].x, objects[i].y, objects[i].width, objects[i].height);
		outDetectedObjectsCount++;
		
		// If we've hit our max object count, stop looping through the objects.
		if (outDetectedObjectsCount == maxOutObjectsCount)
			break;
	}

	// Select the largest detected object, draw a box around it, and identify it as the patient.
	if (maxArea > 0) {

		// Set the Patient Box, which will be used/tracked in other functions.
		patientBox = Rect2d(x, y, w, h);
		patientSet = true;

		// Draw the box and add text
		rectangle(frame, patientBox, Scalar(255, 0, 137), 3, 1);
		putText(frame, "Patient", Point(((patientBox.x + patientBox.width)), (patientBox.y + patientBox.height + 20)), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 0, 137), 2);

		// Create a tracker using the frame and the largest detected object
		tracker = TrackerCSRT::create();
		bool trackerInit = tracker->init(frame, patientBox);
	}

	// Save all of the detected objects for future use
	Objects = objects;

	// Create the window name and show the output
	putText(frame, _windowName + trackerType, Point(120, 40), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(255, 0, 137), 2);
	cv::imshow("Output", frame);
}

// Expose the function for DLL
extern "C" void __declspec(dllexport) __stdcall  Track(Rectangle * outTracking, int maxOutTrackingCount, int& outDetectedTrackingCount)
{
	// Run Track if the Patient is Set
	if (patientSet) {

		// Grab the current frame from the webcam, and return if it's empty.  
		Mat frame;
		_capture >> frame;
		if (frame.empty()) {
			return;
		}

		// Update the tracking result
		bool ok = tracker->update(frame, patientBox);

		// If the tracker returns true (detected object) then draw a rectangle 
		if (ok)
		{
			// Tracking success : Draw the tracked object
			rectangle(frame, patientBox, Scalar(255, 0, 137), 3, 1);
			putText(frame, "Patient", Point(((patientBox.x + patientBox.width)), (patientBox.y + patientBox.height + 20)), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 0, 137), 2);

			// Return the data to Unity (Could be multiple objects if required) 
			outTracking[0] = Rectangle(patientBox.width, patientBox.height, patientBox.x, patientBox.y);
		}
		else
		{
			// Tracking failure detected. Redetect the patient 
			int outDetectedObjectsCount = 0;
			Rectangle* outObjects{};
			Detect(outObjects, maxObjects, outDetectedObjectsCount);
		}

		// Display tracker type on frame
		putText(frame, _windowName + trackerType, Point(120, 40), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(255, 0, 137), 2);

		// Display frame.
		cv::imshow("Output", frame);
	}

}

// Close the Webcam and close any additional windows
extern "C" void __declspec(dllexport) __stdcall  Close()
{
	_capture.release();
}


