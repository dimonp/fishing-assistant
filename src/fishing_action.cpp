
module;

#include <chrono>
#include <exception>
#include <expected>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <robot/Robot.h>

module app.fishing_action;

import app.key_parser;

namespace app {

namespace {

constexpr int bobber_class_id = 0;
constexpr int splash_class_id = 1;

} // namespace

class FishingAction::Impl {
public:
    Impl(
        Controller& controller,
        const DisplayManager& display,
        const FishingConfig& config
    )
        : controller_{controller}
        , display_{display}
        , config_{config}
        , cast_key_{
            config.cast_key.empty()
                ? std::nullopt
                : std::optional<robot::Key>{
                    app::parse_key(config.cast_key)
                }
        }
    {
    }

    void set_region_origin(int x, int y) noexcept
    {
        region_x_ = x;
        region_y_ = y;
    }

    auto process(
        const std::vector<Detection>& detections
    ) -> std::expected<void, std::string>
    {
        const auto now = std::chrono::steady_clock::now();
        const auto observation = observe(detections);

        switch (state_) {
        case State::ReadyToCast: {
            if (observation.bobber_detected ||
                observation.splash_center.has_value()) {
                state_ = State::WaitingForSplash;
                splash_deadline_ =
                    now + config_.splash_timeout_ms;
                return {};
            }

            auto result = cast();
            if (!result) {
                return std::unexpected{result.error()};
            }

            bobber_deadline_ =
                std::chrono::steady_clock::now() +
                config_.bobber_timeout_ms;

            state_ = State::WaitingForBobber;
            return {};
        }

        case State::WaitingForBobber: {
            if (observation.splash_center.has_value()) {
                const auto [x, y] = *observation.splash_center;

                auto result = handle_splash(x, y);
                if (!result) {
                    return std::unexpected{result.error()};
                }

                next_cast_time_ =
                    std::chrono::steady_clock::now() +
                    config_.cast_cooldown_ms;

                state_ = State::WaitingForNextCast;
                return {};
            }

            if (observation.bobber_detected) {
                splash_deadline_ =
                    now + config_.splash_timeout_ms;

                state_ = State::WaitingForSplash;
                return {};
            }

            if (now >= bobber_deadline_) {
                state_ = State::ReadyToCast;
            }

            return {};
        }

        case State::WaitingForSplash: {
            if (observation.splash_center.has_value()) {
                const auto [x, y] = *observation.splash_center;

                auto result = handle_splash(x, y);
                if (!result) {
                    return std::unexpected{result.error()};
                }

                next_cast_time_ =
                    std::chrono::steady_clock::now() +
                    config_.cast_cooldown_ms;

                state_ = State::WaitingForNextCast;
                return {};
            }

            if (!observation.bobber_detected ||
                now >= splash_deadline_) {
                state_ = State::ReadyToCast;
            }

            return {};
        }

        case State::WaitingForNextCast: {
            if (now >= next_cast_time_) {
                state_ = State::ReadyToCast;
            }

            return {};
        }
        }

        return std::unexpected{
            std::string{"Invalid fishing state"}
        };
    }

private:
    enum class State {
        ReadyToCast,
        WaitingForBobber,
        WaitingForSplash,
        WaitingForNextCast
    };

    struct Observation {
        bool bobber_detected{};
        std::optional<std::pair<int, int>> splash_center;
    };

    [[nodiscard]]
    auto observe(
        const std::vector<Detection>& detections
    ) const -> Observation
    {
        Observation result;

        for (const auto& detection : detections) {
            if (detection.class_id == splash_class_id) {
                result.splash_center = std::pair{
                    detection.box.x + detection.box.width / 2,
                    detection.box.y + detection.box.height / 2
                };
            } else if (detection.class_id == bobber_class_id) {
                result.bobber_detected = true;
            }
        }

        return result;
    }

    [[nodiscard]]
    auto cast() -> std::expected<void, std::string>
    {
        try {
             if (cast_key_.has_value()) {
                controller_.tap_key(*cast_key_);
            } else {
                controller_.double_click_right();
            }

            std::cout << "Fishing cast\n";
            return {};
        } catch (const std::exception& error) {
            return std::unexpected{
                std::string{"Failed to cast: "} + error.what()
            };
        }
    }

    [[nodiscard]]
    auto handle_splash(
        int center_x,
        int center_y
    ) -> std::expected<void, std::string>
    {
        const auto [screen_x, screen_y] =
            display_.physical_to_logical({
                region_x_ + center_x,
                region_y_ + center_y
            });

        try {
            std::cout
                << "Splash detected: x=" << screen_x
                << " y=" << screen_y
                << '\n';

            controller_.move_smooth(screen_x, screen_y);

            controller_.key_down(robot::Key::LeftShift);
            controller_.click_right();
            controller_.key_up(robot::Key::LeftShift);

            return {};
        } catch (const std::exception& error) {
            return std::unexpected{
                std::string{"Failed to handle splash: "} +
                error.what()
            };
        }
    }

    Controller& controller_;
    const DisplayManager& display_;
    const FishingConfig& config_;

    std::optional<robot::Key> cast_key_;

    int region_x_{};
    int region_y_{};

    State state_{State::ReadyToCast};

    std::chrono::steady_clock::time_point bobber_deadline_{};
    std::chrono::steady_clock::time_point splash_deadline_{};
    std::chrono::steady_clock::time_point next_cast_time_{};
};

FishingAction::FishingAction(
    Controller& controller,
    const DisplayManager& display,
    const FishingConfig& config
)
    : impl_{
        std::make_unique<Impl>(controller, display, config)
    }
{
}

FishingAction::~FishingAction() = default;

FishingAction::FishingAction(FishingAction&&) noexcept = default;

auto FishingAction::operator=(FishingAction&&) noexcept
    -> FishingAction& = default;

void FishingAction::set_region_origin(int x, int y) noexcept
{
    impl_->set_region_origin(x, y);
}

auto FishingAction::process(
    const std::vector<Detection>& detections
) -> std::expected<void, std::string>
{
    return impl_->process(detections);
}

} // namespace app
