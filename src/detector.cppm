module;

#include <expected>
#include <memory>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

export module app.detector;

import app.config;

export namespace app {

struct Detection {
    cv::Rect2f box;
    float confidence{};
    int class_id{};
};

struct DetectorError {
    std::string message;
};

class Detector {
public:
    Detector();
    ~Detector();

    Detector(Detector&&) noexcept;
    auto operator=(Detector&&) noexcept -> Detector&;

    Detector(const Detector&) = delete;
    auto operator=(const Detector&) -> Detector& = delete;

    [[nodiscard]]
    auto load(
        const ModelConfig& config,
        float iou_threshold = 0.45F
    ) -> std::expected<void, DetectorError>;

    [[nodiscard]]
    auto detect(
        const cv::Mat& frame
    ) -> std::expected<std::vector<Detection>, DetectorError>;

    [[nodiscard]]
    auto is_loaded() const noexcept -> bool;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace app
