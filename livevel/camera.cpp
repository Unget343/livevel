#include "camera.h"

namespace livevel {

Camera::Camera(int deviceId) : deviceId_(deviceId) {}

Camera::~Camera() {
    if (cap_.isOpened()) {
        cap_.release();
    }
}

bool Camera::open() {
    cap_.open(deviceId_);
    return cap_.isOpened();
}

bool Camera::read(cv::Mat& frame) {
    if (!cap_.isOpened()) return false;
    cap_ >> frame;
    return !frame.empty();
}

bool Camera::isOpened() const {
    return cap_.isOpened();
}

} // namespace livevel
