module;

#include <string>
#include <string_view>
#include <vector>

#include <opencv2/core.hpp>

export module app.preview;

import app.config;
import app.detector;

export namespace app
{

class Preview
{
public:
    explicit Preview(const DisplayConfig& config);

    ~Preview();

    Preview(const Preview&) = delete;
    auto operator=(const Preview&) -> Preview& = delete;

    Preview(Preview&&) = delete;
    auto operator=(Preview&&) -> Preview& = delete;

    void render(
        cv::Mat& frame,
        const std::vector<Detection>& detections,
        const std::vector<std::string>& classes,
        double fps
    );

    void render_stopped(
        cv::Mat& frame,
        const std::string_view hint
    );

    void poll_events();

private:
    bool enabled_{false};
    DisplayConfig config_;
};

} // namespace app