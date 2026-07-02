#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/video.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

int main()
{
	cv::VideoCapture cap(0);

	if (!cap.isOpened()) return 5;

	cv::Mat frame;
	cap >> frame;

	if (frame.empty()) return 6;

	// Выбор области для трекинга
	cv::Rect roi = cv::selectROI("livevel", frame, false, false);
	if (roi.empty()) return 7;

	cv::destroyWindow("livevel");

	// Создание трекера (TrackerMIL — не требует DNN-моделей)
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

		if (cv::waitKey(1) == 27) break; // ESC для выхода
	}

	return 0;
}
