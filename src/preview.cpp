module;

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

module app.preview;

namespace app
{

namespace
{

[[nodiscard]] auto get_class_name(
    const std::vector<std::string>& classes,
    const int class_id
) -> std::string
{
    if (class_id < 0 ||
        static_cast<std::size_t>(class_id) >= classes.size()) {
        return "class_" + std::to_string(class_id);
    }

    return classes[static_cast<std::size_t>(class_id)];
}

void draw_detections(
    cv::Mat& frame,
    const std::vector<Detection>& detections,
    const std::vector<std::string>& classes,
    const bool show_confidence
)
{
    for (const auto& detection : detections) {
        const cv::Rect box{
            static_cast<int>(std::lround(detection.box.x)),
            static_cast<int>(std::lround(detection.box.y)),
            static_cast<int>(std::lround(detection.box.width)),
            static_cast<int>(std::lround(detection.box.height))
        };

        cv::rectangle(
            frame,
            box,
            cv::Scalar{0, 255, 0},
            2
        );

        std::string label =
            get_class_name(classes, detection.class_id);

        if (show_confidence) {
            label += " " +
                std::to_string(detection.confidence).substr(0, 4);
        }

        const int text_y = std::max(20, box.y - 8);

        cv::putText(
            frame,
            label,
            {box.x, text_y},
            cv::FONT_HERSHEY_SIMPLEX,
            0.6,
            cv::Scalar{0, 255, 0},
            2,
            cv::LINE_AA
        );
    }
}

void draw_fps(cv::Mat& frame, const double fps)
{
    const std::string label =
        "FPS: " +
        std::to_string(static_cast<int>(std::lround(fps)));

    cv::putText(
        frame,
        label,
        {10, 30},
        cv::FONT_HERSHEY_SIMPLEX,
        0.8,
        cv::Scalar{0, 255, 255},
        2,
        cv::LINE_AA
    );
}

} // namespace


Preview::Preview(const DisplayConfig& config)
    : enabled_{config.show_preview}
    , config_{config}
{
    if (enabled_) {
        cv::namedWindow(config_.window_name, cv::WINDOW_NORMAL);
    }
}

Preview::~Preview()
{
    if (enabled_) {
        cv::destroyWindow(config_.window_name);
    }
}


void Preview::render(
    cv::Mat& frame,
    const std::vector<Detection>& detections,
    const std::vector<std::string>& classes,
    const double fps
)
{
    if (!enabled_) {
        return;
    }

    draw_detections(
        frame,
        detections,
        classes,
        config_.show_confidence
    );

    if (config_.show_fps) {
        draw_fps(frame, fps);
    }

    cv::imshow(config_.window_name, frame);
}

void Preview::poll_events()
{
    if (enabled_) {
        cv::waitKey(1);
    }
}
} // namespace app
