module;

#include <memory>

#include <opencv2/core.hpp>

export module app.screen_capture;

import app.robot_context;
import app.display_manager;

export namespace app
{

struct CaptureRegion
{
    int x{};
    int y{};
    int width{};
    int height{};
};

class ScreenCapture
{
public:
    ScreenCapture(
        RobotContext& context,
        const DisplayManager& display
    );

    ~ScreenCapture();

    ScreenCapture(const ScreenCapture&) = delete;
    auto operator=(const ScreenCapture&) -> ScreenCapture& = delete;

    ScreenCapture(ScreenCapture&&) noexcept;
    auto operator=(ScreenCapture&&) noexcept -> ScreenCapture&;

    void set_region(CaptureRegion region);

    [[nodiscard]]
    auto region() const noexcept -> CaptureRegion;

    [[nodiscard]]
    auto capture_monitor() -> cv::Mat;

    [[nodiscard]]
    auto capture() -> cv::Mat;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    CaptureRegion region_{};
};

} // namespace app