#pragma once
#include <opencv2/core.hpp>
#include <string>

namespace livevel {

class Renderer {
public:
    Renderer(const std::string& windowName);
    ~Renderer();

    void render(const cv::Mat& frame);

private:
    std::string windowName_;
};

} // namespace livevel
