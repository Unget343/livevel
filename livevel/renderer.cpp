#include "renderer.h"
#include <opencv2/highgui.hpp>

namespace livevel {

Renderer::Renderer(const std::string& windowName) : windowName_(windowName) {
    cv::namedWindow(windowName_, cv::WINDOW_AUTOSIZE);
}

Renderer::~Renderer() {
    cv::destroyWindow(windowName_);
}

void Renderer::render(const cv::Mat& frame) {
    if (!frame.empty()) {
        cv::imshow(windowName_, frame);
    }
}

} // namespace livevel
