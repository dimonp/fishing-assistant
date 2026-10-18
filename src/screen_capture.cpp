module;

#include <cstddef>
#include <memory>
#include <stdexcept>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <robot/Robot.h>

module app.screen_capture;

import app.display_manager;

namespace app {

namespace {

[[nodiscard]]
auto to_cv_mat(const robot::Image& image) -> cv::Mat
{
    const auto width = image.size().width;
    const auto height = image.size().height;

    if (image.empty() || width <= 0 || height <= 0) {
        return {};
    }

    const auto& pixels = image.pixels();

    const auto expected_size =
        static_cast<std::size_t>(width) *
        static_cast<std::size_t>(height);

    if (pixels.size() != expected_size) {
        throw std::runtime_error{
            "Pixel count does not match image dimensions"
        };
    }

    cv::Mat rgba(
        height,
        width,
        CV_8UC4,
        const_cast<robot::Rgba*>(pixels.data())
    );

    cv::Mat bgr;
    cv::cvtColor(rgba, bgr, cv::COLOR_RGBA2BGR);

    return bgr;
}

[[nodiscard]]
auto is_valid_region(
    const CaptureRegion& region,
    const DisplayInfo& display
) -> bool
{
    if (region.x < 0 || region.y < 0 ||
        region.width <= 0 || region.height <= 0) {
        return false;
    }

    return region.width <= display.physical_width &&
           region.height <= display.physical_height &&
           region.x <= display.physical_width - region.width &&
           region.y <= display.physical_height - region.height;
}

[[nodiscard]]
auto make_physical_rect(
    const CaptureRegion& region,
    const DisplayInfo& display
) -> robot::PhysicalRect
{
    return robot::PhysicalRect{
        .origin = {
            .x = display.physical_x + region.x,
            .y = display.physical_y + region.y
        },
        .size = {
            .width = region.width,
            .height = region.height
        }
    };
}

} // namespace

class ScreenCapture::Impl {
public:
    Impl(
        RobotContext& context,
        const DisplayManager& display
    )
        : session{context.session()}
        , display{display}
    {
    }

    robot::Session& session;
    const DisplayManager& display;
};

ScreenCapture::ScreenCapture(
    RobotContext& context,
    const DisplayManager& display
)
    : impl_{std::make_unique<Impl>(context, display)}
{
}

ScreenCapture::~ScreenCapture() = default;

ScreenCapture::ScreenCapture(ScreenCapture&&) noexcept = default;

auto ScreenCapture::operator=(ScreenCapture&&) noexcept
    -> ScreenCapture& = default;

void ScreenCapture::set_region(CaptureRegion region)
{
    if (region.x < 0 || region.y < 0 ||
        region.width <= 0 || region.height <= 0) {
        throw std::invalid_argument{"Invalid capture region"};
    }

    region_ = region;
}

auto ScreenCapture::region() const noexcept -> CaptureRegion
{
    return region_;
}

auto ScreenCapture::capture_monitor() -> cv::Mat
{
    const auto display = impl_->display.display_info();

    const auto image = impl_->session.screen().captureMonitor(
        impl_->session.screen().monitors()
            .value()[static_cast<std::size_t>(display.index)].id
    );

    if (!image) {
        throw std::runtime_error{"Failed to capture monitor"};
    }

    return to_cv_mat(*image);
}

auto ScreenCapture::capture() -> cv::Mat
{
    const auto display = impl_->display.display_info();

    if (!is_valid_region(region_, display)) {
        throw std::out_of_range{
            "Capture region is outside the selected monitor"
        };
    }

    const auto physical_region =
        make_physical_rect(region_, display);

    const auto image =
        impl_->session.screen().capture(physical_region);

    if (!image) {
        throw std::runtime_error{"Failed to capture region"};
    }

    return to_cv_mat(*image);
}

} // namespace app