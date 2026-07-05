#include <iostream>

#if LIVEVEL_HAVE_OPENCV
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/videoio.hpp>
#endif

int main()
{
#if LIVEVEL_HAVE_OPENCV
    cv::VideoCapture cap(0);

    if (!cap.isOpened()) return 5;

    cv::Mat frame;
    cap >> frame;

    if (frame.empty()) return 6;

    // Select the area to track.
    cv::Rect roi = cv::selectROI("livevel", frame, false, false);
    if (roi.empty()) return 7;

    cv::destroyWindow("livevel");

    cv::Ptr<cv::TrackerMIL> tracker = cv::TrackerMIL::create();
    tracker->init(frame, roi);

    while (cap.isOpened())
    {
        cap >> frame;
        if (frame.empty()) break;

        bool found = tracker->update(frame, roi);

        if (found)
        {
            cv::rectangle(frame, roi, cv::Scalar(0, 255, 0), 2);
        }

        cv::imshow("livevel", frame);

        if (cv::waitKey(1) == 27) break; // ESC
    }

    return 0;
#else
    std::cerr << "livevel was built without OpenCV. Install OpenCV and reconfigure CMake with OpenCV_DIR or CMAKE_PREFIX_PATH.\n";
    return 2;
#endif
}
