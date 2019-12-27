/*
 * Windows only example
 */
#include <iostream>

// windows libraries
#include <Windows.h>

// openCV 4.1.1 libraries
#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"

using namespace std;
using namespace cv;

Mat hwnd2mat(HWND hwnd);
void detectAndDraw(Mat& img, CascadeClassifier& cascade,
		CascadeClassifier& nestedCascade, double scale, bool tryflip);
string cascadeName;
string nestedCascadeName;

int main(int argc, const char** argv) {
	VideoCapture capture;
	Mat frame, croppedImage;
	string inputName;
	bool tryflip;
	CascadeClassifier cascade, nestedCascade;
	double scale;

	cascadeName = "data/haarcascades/haarcascade_frontalface_alt.xml";
	nestedCascadeName = "data/haarcascades/haarcascade_eye_tree_eyeglasses.xml";
	scale = 1;
	tryflip = false;

	if (!nestedCascade.load(samples::findFileOrKeep(nestedCascadeName)))
		cerr << "WARNING: Could not load classifier cascade for nested objects"
				<< endl;
	if (!cascade.load(samples::findFile(cascadeName))) {
		cerr << "ERROR: Could not load classifier cascade" << endl;
		return -1;
	}

	HWND hWndDesktop = GetDesktopWindow();
	frame = hwnd2mat(hWndDesktop);
	int frameWidth = frame.cols;
	int frameX = frameWidth - 720;
	cout <<frameWidth;

	// Setup a rectangle to define your region of interest
	cv::Rect myROI(frameX, 0, 720, 480);

	cout << "Desktop capturing has been started ..." << endl;
	cv::namedWindow( "result", WINDOW_NORMAL );
	resizeWindow("result", 720, 480);
	for (;;) {
		HWND hWndDesktop = GetDesktopWindow();
		frame = hwnd2mat(hWndDesktop);

		// Crop the full image to that image contained by the rectangle myROI
		// Note that this doesn't copy the data
		croppedImage = frame(myROI);
		detectAndDraw(croppedImage, cascade, nestedCascade, scale, tryflip);
		char c = (char) waitKey(10);
		if (c == 27 || c == 'q' || c == 'Q')
			break;
	}
	system("pause");

	return 0;
}
void detectAndDraw(Mat& img, CascadeClassifier& cascade,
		CascadeClassifier& nestedCascade, double scale, bool tryflip) {
	double t = 0;
	vector<Rect> faces, faces2;
	const static Scalar colors[] = { Scalar(255, 0, 0), Scalar(255, 128, 0),
			Scalar(255, 255, 0), Scalar(0, 255, 0), Scalar(0, 128, 255), Scalar(
					0, 255, 255), Scalar(0, 0, 255), Scalar(255, 0, 255) };
	Mat gray, smallImg;
	cvtColor(img, gray, COLOR_BGR2GRAY);
	double fx = 1 / scale;
	resize(gray, smallImg, Size(), fx, fx, INTER_LINEAR_EXACT);
	equalizeHist(smallImg, smallImg);
	t = (double) getTickCount();
	cascade.detectMultiScale(smallImg, faces, 1.1, 2, 0
	//|CASCADE_FIND_BIGGEST_OBJECT
	//|CASCADE_DO_ROUGH_SEARCH
			| CASCADE_SCALE_IMAGE, Size(30, 30));
	if (tryflip) {
		flip(smallImg, smallImg, 1);
		cascade.detectMultiScale(smallImg, faces2, 1.1, 2, 0
		//|CASCADE_FIND_BIGGEST_OBJECT
		//|CASCADE_DO_ROUGH_SEARCH
				| CASCADE_SCALE_IMAGE, Size(30, 30));
		for (vector<Rect>::const_iterator r = faces2.begin(); r != faces2.end();
				++r) {
			faces.push_back(
					Rect(smallImg.cols - r->x - r->width, r->y, r->width,
							r->height));
		}
	}
	t = (double) getTickCount() - t;
	printf("detection time = %g ms\n", t * 1000 / getTickFrequency());
	for (size_t i = 0; i < faces.size(); i++) {
		Rect r = faces[i];
		Mat smallImgROI;
		vector<Rect> nestedObjects;
		Point center;
		Scalar color = colors[i % 8];
		int radius;
		double aspect_ratio = (double) r.width / r.height;
		if (0.75 < aspect_ratio && aspect_ratio < 1.3) {
			center.x = cvRound((r.x + r.width * 0.5) * scale);
			center.y = cvRound((r.y + r.height * 0.5) * scale);
			radius = cvRound((r.width + r.height) * 0.25 * scale);
			circle(img, center, radius, color, 3, 8, 0);
		} else
			rectangle(img, Point(cvRound(r.x * scale), cvRound(r.y * scale)),
					Point(cvRound((r.x + r.width - 1) * scale),
							cvRound((r.y + r.height - 1) * scale)), color, 3, 8,
					0);
		if (nestedCascade.empty())
			continue;
		smallImgROI = smallImg(r);
		nestedCascade.detectMultiScale(smallImgROI, nestedObjects, 1.1, 2, 0
		//|CASCADE_FIND_BIGGEST_OBJECT
		//|CASCADE_DO_ROUGH_SEARCH
		//|CASCADE_DO_CANNY_PRUNING
				| CASCADE_SCALE_IMAGE, Size(30, 30));
		for (size_t j = 0; j < nestedObjects.size(); j++) {
			Rect nr = nestedObjects[j];
			center.x = cvRound((r.x + nr.x + nr.width * 0.5) * scale);
			center.y = cvRound((r.y + nr.y + nr.height * 0.5) * scale);
			radius = cvRound((nr.width + nr.height) * 0.25 * scale);
			circle(img, center, radius, color, 3, 8, 0);
		}
	}
	imshow("result", img);
}

Mat hwnd2mat(HWND hwnd) {
	HDC hwindowDC, hwindowCompatibleDC;

	int height, width, srcheight, srcwidth;
	HBITMAP hbwindow;
	Mat src;
	BITMAPINFOHEADER bi;

	hwindowDC = GetDC(hwnd);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	RECT windowsize;    // get the height and width of the screen
	GetClientRect(hwnd, &windowsize);

	srcheight = windowsize.bottom;
	srcwidth = windowsize.right;
	height = windowsize.bottom / 1; //change this to whatever size you want to resize to
	width = windowsize.right / 1;

	src.create(height, width, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER); //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = width;
	bi.biHeight = -height; //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0,
			srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data,
			(BITMAPINFO *) &bi, DIB_RGB_COLORS); //copy from hwindowCompatibleDC to hbwindow

	// avoid memory leak
	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(hwnd, hwindowDC);

	return src;
}
