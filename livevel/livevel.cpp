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

	while (!frame.empty())
	{
		cap >> frame;

		// Проверяем, что кадр получен
		if (frame.empty())
			break;

		// Показываем кадр
		cv::imshow("Camera", frame);

		// Выход по клавише Esc
		int key = cv::waitKey(1);
		if (key == 27)
			break;
	}

	cap.release();
	cv::destroyAllWindows();

	return 0;
}
