#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/dir_nav.h>
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
using namespace dlib;


// OBJECT TRACKING PROTOTYPE



// Convert to string
#define SSTR( x ) static_cast< std::ostringstream & >( \
( std::ostringstream() << std::dec << x ) ).str()

// Object Tracking Rectangles (Renamed BodyBox after Dlib Import)!
struct BodyBox {
	BodyBox(int width, int height, int x, int y) : Width(width), Height(height), X(x), Y(y) {}
	int Width, Height, X, Y;
};

CascadeClassifier _bodyCascade;
String _windowName = "Unity OpenCV Prototype #2";
VideoCapture _capture;
int _scale = 1;
correlation_tracker tracker;
std::vector<correlation_tracker> faceTrackers;
std::vector<String> faceNames;
string trackerType;
Rect2d patientBox;
bool patientSet;
int currentFaceID = 0;
int frameCounter = 0;
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


	//Create two opencv named windows
		cv::namedWindow("Input", cv::WINDOW_AUTOSIZE);
		cv::namedWindow("Output", cv::WINDOW_AUTOSIZE);

		//Position the windows next to eachother
		cv::moveWindow("Input", 0, 100);
		cv::moveWindow("Output", 400, 100);

		//Start the window thread for the two windows we are using
		cv::startWindowThread();


		// Variables holding the current frame number and the current faceid
		frameCounter = 0;
		currentFaceID = 0;

		// Variables holding the correlation trackers and the name per faceid - cleared for init
		faceTrackers.clear();
		faceNames.clear();


}
extern "C" void __declspec(dllexport) __stdcall  RecognizePerson(size_t id)
{
	faceNames[id] = ("Person " + id);
}
extern "C" void __declspec(dllexport) __stdcall Detect(BodyBox * outBodies, int maxOutBodiesCount, int& outDetectedBodiesCount)
{
	Mat frame;
	// >> shifts right and adds either 0s, if value is an unsigned type, or extends the top bit (to preserve the sign) if its a signed type.
	_capture >> frame;
	if (frame.empty())
		return;

	frameCounter += 1;



	std::vector<int> idsToDelete;
	std::vector<Rect> bodies;
	std::vector<Rect> keys;


	for (size_t i = 0; i < faceTrackers.size(); i++) {
			int trackingQuality = faceTrackers[i].update(frame);

			//If the tracking quality is good enough, we must delete
		//this tracker
			if (trackingQuality < 7) {
				idsToDelete.push_back(i);
			}

			for (size_t i = 0; i < idsToDelete.size(); i++) {
				size_t id = idsToDelete[i];
				faceTrackers.erase(faceTrackers.erase(faceTrackers.begin() + id));
			}
		
	}
	// Every Ten Frames, rerun the cascade to ensure we're still tracking the selected object
	if (frameCounter % 10 == 0) {
		// Convert the frame to grayscale for cascade detection.
		Mat grayscaleFrame;
		cvtColor(frame, grayscaleFrame, COLOR_BGR2GRAY);
		Mat resizedGray;
		// Scale down for better performance.
		resize(grayscaleFrame, resizedGray, Size(frame.cols / _scale, frame.rows / _scale));
		equalizeHist(resizedGray, resizedGray);

		// Detect faces.
		_bodyCascade.detectMultiScale(resizedGray, bodies);

		// Draw faces.
		for (size_t i = 0; i < bodies.size(); i++)
		{
			// Set up coordinates for center detection 
				int x = bodies[i].x;
				int y = bodies[i].y;
				int w = bodies[i].width;
				int h = bodies[i].height;

				// calculate the centerpoint
				int	x_bar = x + 0.5 * w;
				int	y_bar = y + 0.5 * h;

				correlation_tracker matchedid;
				bool matched = false;

				// Loop through the trackers and check if the center point is in the box of a tracker
				for (size_t i = 0; i < faceTrackers.size(); i++) {
					drectangle tracked_position = faceTrackers[i].get_position();

					// Set the Tracker coordinates
					int t_x = int(tracked_position.left());
					int t_y = int(tracked_position.top());
					int t_w = int(tracked_position.width());
					int t_h = int(tracked_position.height());
					
					//calculate the centerpoint
					int t_x_bar = t_x + 0.5 * t_w;
					int t_y_bar = t_y + 0.5 * t_h;

					// Check if the center point of the face is within the rectangle of the tracker & the region detected as a face
					// face fit
					bool fxfit = (t_x <= x_bar <= (t_x + t_w));
					bool fyfit = (t_y <= y_bar <= (t_y + t_h));
					// current track fit
					bool txfit = (x <= t_x_bar <= (x + w));
					bool tyfit = (y <= t_y_bar <= (y + h));

					if (fxfit && fyfit && txfit && tyfit) {
						matchedid = faceTrackers[i];
						matched = true;
					}

					if (!matched) {
						print("Creating new tracker " + currentFaceID);

						// Create + store the tracker
						tracker = dlib::correlation_tracker();
						tracker.start_track(frame,
							dlib::rectangle(x - 10,
								y - 20,
								x + w + 10,
								y + h + 20));

						faceTrackers[currentFaceID] = tracker;

							//Recognize the face 
							RecognizePerson(currentFaceID);

							// Increase the currentFaceID counter
							currentFaceID += 1;



					}
					
				}

			// Send to application.
			outBodies[i] = BodyBox(bodies[i].x, bodies[i].y, bodies[i].width, bodies[i].height);
			outDetectedBodiesCount++;

			if (outDetectedBodiesCount == maxOutBodiesCount)
				break;
		}
		Bodies = bodies;
	}

	for (size_t i = 0; i < faceTrackers.size(); i++) {
		drectangle tracked_position = faceTrackers[i].get_position();

			int t_x = int(tracked_position.left());
			int t_y = int(tracked_position.top());
			int t_w = int(tracked_position.width());
			int t_h = int(tracked_position.height());
			cv::Point pt1(t_x, t_y);
			cv::Point pt2((t_x + t_w), (t_y + t_h));
			cv::rectangle(frame, pt1, pt2, Scalar(255, 0, 0), 1);


			if (i < faceNames.size()) {
				cv::putText(frame, faceNames[i],
					(cv::Point(t_x + t_w / 2), cv::Point(t_y)),
					cv::FONT_HERSHEY_SIMPLEX,
					0.5, (255, 255, 255), 2);
			}
			else {
				cv::putText(frame, "Detecting...",
					(cv::Point(t_x + t_w / 2), cv::Point(t_y)),
					cv::FONT_HERSHEY_SIMPLEX,
					0.5, (255, 255, 255), 2);
			}


		cv::imshow("Output", frame);

}
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

	// Draw faces.
	for (size_t i = 0; i < bodies.size(); i++)
	{
		cv::rectangle(frame, bodies[i], Scalar(255, 0, 0), 2, 1);
	}
	Bodies = bodies;
	patientBox = Bodies[0];

	// Display debug output.
	cv::imshow("tracker", frame);
}

// Expose the function for DLL /*
extern "C" void __declspec(dllexport) __stdcall  Track(BodyBox * outTracking, int maxOutTrackingCount, int& outDetectedTrackingCount)
{
	if (patientSet) {
		Mat frame;
		_capture >> frame;
		if (frame.empty()) {
			return;
		}

		// Update the tracking result
		//bool ok = tracker->update(frame, patientBox);

		if (true)
		{
			// Tracking success : Draw the tracked object
			cv::rectangle(frame, patientBox, Scalar(255, 0, 0), 2, 1);
			outTracking[0] = BodyBox(patientBox.width, patientBox.height, patientBox.x, patientBox.y);
		}
		else
		{
			// Tracking failure detected.
			RefindPatient();
		}

		// Display tracker type on frame
		putText(frame, trackerType + " Tracker", Point(100, 20), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50, 170, 50), 2);

		// Display frame.
		cv::imshow("Tracking", frame);
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
