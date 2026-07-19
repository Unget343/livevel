#pragma once
#include <opencv2/videoio.hpp>

namespace livevel {

class Camera {
public:
    Camera(int deviceId = 0);
    ~Camera();

    bool open();
    bool read(cv::Mat& frame);
    bool isOpened() const;

private:
    int deviceId_;
    cv::VideoCapture cap_;
};

} // namespace livevel
