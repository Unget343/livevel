#include <opencv4/opencv2/opencv.hpp>
#include <iostream>

int main() 
{
    cv::VideoCapture cap(0);
    if (!cap.isOpened())
    {
        std::cerr << "Cannot open camera" << std::endl;
        return 1;
    }
}