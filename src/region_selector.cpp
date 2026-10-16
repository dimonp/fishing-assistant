module;

#include <iostream>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

module app.region_selector;

namespace app
{

auto select_capture_region(ScreenCapture& screen) -> CaptureRegion
{
    const cv::Mat frame = screen.capture_monitor();

    if (frame.empty()) {
        std::cerr << "Cannot select region: monitor capture failed.\n";
        return {};
    }

    const std::string window_name = "Select capture region";

    cv::namedWindow(window_name, cv::WINDOW_NORMAL);
    cv::setWindowProperty(
        window_name,
        cv::WND_PROP_FULLSCREEN,
        cv::WINDOW_FULLSCREEN
    );
    cv::setWindowProperty(
        window_name,
        cv::WND_PROP_TOPMOST,
        1.0
    );

    const cv::Rect selected = cv::selectROI(
        window_name,
        frame,
        false,
        false
    );

    cv::destroyWindow(window_name);

    if (selected.width <= 0 || selected.height <= 0) {
        return {};
    }

    return CaptureRegion{
        selected.x,
        selected.y,
        selected.width,
        selected.height
    };
}

} // namespace app