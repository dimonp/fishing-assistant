
module;

#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>

#include <robot/Robot.h>

module app.hotkeys;

import app.key_parser;

namespace {

constexpr std::size_t key_state_count = 256;

[[nodiscard]] auto modifier_for_key(
    robot::Key key
) -> app::ModifierMask
{
    using enum robot::Key;

    switch (key) {
    case LeftControl:
    case RightControl:
        return app::modifier_ctrl;

    case LeftShift:
    case RightShift:
        return app::modifier_shift;

    case LeftAlt:
    case RightAlt:
        return app::modifier_alt;

    case LeftMeta:
    case RightMeta:
        return app::modifier_meta;

    default:
        return 0;
    }
}

} // namespace

class app::HotkeyListener::Impl
{
public:
    Impl(
        RobotContext& context,
        const std::string& start_key_name,
        const std::string& stop_key_name,
        const std::string& configure_region_key_name,
        const std::string& quit_key_name
    )
        : tap_(context.session().eventTap()),
          start_hotkey_(parse_hotkey(start_key_name)),
          stop_hotkey_(parse_hotkey(stop_key_name)),
          configure_region_hotkey_(parse_hotkey(configure_region_key_name)),
          quit_key_(parse_hotkey(quit_key_name))
    {
        if (start_hotkey_.key == stop_hotkey_.key &&
            start_hotkey_.modifiers == stop_hotkey_.modifiers) {
            throw std::invalid_argument(
                "Start and stop hotkeys must be different.");
        }

        if (start_hotkey_.key == configure_region_hotkey_.key &&
            start_hotkey_.modifiers ==
                configure_region_hotkey_.modifiers) {
            throw std::invalid_argument(
                "Start and configure-region hotkeys must be different.");
        }

        if (stop_hotkey_.key == configure_region_hotkey_.key &&
            stop_hotkey_.modifiers ==
                configure_region_hotkey_.modifiers) {
            throw std::invalid_argument(
                "Stop and configure-region hotkeys must be different.");
        }
    }

    ~Impl()
    {
        stop();
    }

    [[nodiscard]] auto start() -> bool
    {
        std::lock_guard lifecycle_lock{lifecycle_mutex_};

        {
            std::lock_guard lock{mutex_};

            if (state_ == State::Starting) {
                return true;
            }
        }

        if (thread_.joinable()) {
            thread_.join();
        }

        if (!tap_.isSupported()) {
            std::cerr << "Global keyboard event tap is unsupported.\n";
            return false;
        }

        {
            std::lock_guard lock{mutex_};
            stopping_ = false;
            state_ = State::Starting;
            pressed_.fill(false);
        }

        try {
            thread_ = std::thread([this] {
                auto result = tap_.start(
                    [this](const robot::InputEvent& event) {
                        handle_event(event);
                    });

                if (!result) {
                    std::cerr << "EventTap::start failed: "
                              << result.error().message << '\n';
                }

                std::lock_guard lock{mutex_};
                state_ = State::Stopped;
            });
        } catch (...) {
            std::lock_guard lock{mutex_};
            state_ = State::Stopped;
            stopping_ = false;
            throw;
        }

        return true;
    }

    void stop() noexcept
    {
        std::lock_guard lifecycle_lock{lifecycle_mutex_};

        {
            std::lock_guard lock{mutex_};

            if (!thread_.joinable()) {
                state_ = State::Stopped;
                stopping_ = false;
                return;
            }

            stopping_ = true;
            state_ = State::Stopping;
        }

        tap_.stop();

        if (thread_.get_id() != std::this_thread::get_id()) {
            thread_.join();
        }

        {
            std::lock_guard lock{mutex_};
            state_ = State::Stopped;
            stopping_ = false;
            pressed_.fill(false);
        }
    }

    [[nodiscard]] auto poll_action() noexcept
        -> std::optional<HotkeyAction>
    {
        std::lock_guard lock{mutex_};

        if (actions_.empty()) {
            return std::nullopt;
        }

        const auto action = actions_.front();
        actions_.pop_front();

        return action;
    }

private:
    enum class State {
        Stopped,
        Starting,
        Stopping
    };

    static constexpr std::size_t max_actions = 16;

    [[nodiscard]] auto current_modifiers() const
        -> ModifierMask
    {
        ModifierMask result{};

        const auto is_pressed = [this](robot::Key key) {
            const auto index = static_cast<std::size_t>(key);

            return index < pressed_.size() && pressed_[index];
        };

        if (is_pressed(robot::Key::LeftControl) ||
            is_pressed(robot::Key::RightControl)) {
            result |= modifier_ctrl;
        }

        if (is_pressed(robot::Key::LeftShift) ||
            is_pressed(robot::Key::RightShift)) {
            result |= modifier_shift;
        }

        if (is_pressed(robot::Key::LeftAlt) ||
            is_pressed(robot::Key::RightAlt)) {
            result |= modifier_alt;
        }

        if (is_pressed(robot::Key::LeftMeta) ||
            is_pressed(robot::Key::RightMeta)) {
            result |= modifier_meta;
        }

        return result;
    }

    void enqueue_action(HotkeyAction action)
    {
        if (std::find(actions_.begin(), actions_.end(), action) !=
            actions_.end()) {
            return;
        }

        if (actions_.size() >= max_actions) {
            if (action == HotkeyAction::Stop) {
                const auto it = std::find_if(
                    actions_.begin(),
                    actions_.end(),
                    [](HotkeyAction pending) {
                        return pending != HotkeyAction::Stop;
                    });

                if (it != actions_.end()) {
                    actions_.erase(it);
                } else {
                    return;
                }
            } else {
                return;
            }
        }

        actions_.push_back(action);
    }

    void handle_event(const robot::InputEvent& event)
    {
        const auto* key_event =
            std::get_if<robot::KeyEvent>(&event);

        if (key_event == nullptr) {
            return;
        }

        const auto key_index =
            static_cast<std::size_t>(key_event->key);

        if (key_index >= pressed_.size()) {
            return;
        }

        std::lock_guard lock{mutex_};

        if (stopping_) {
            return;
        }

        const auto modifier = modifier_for_key(key_event->key);

        if (modifier != 0) {
            pressed_[key_index] = key_event->down;
            return;
        }

        if (!key_event->down) {
            pressed_[key_index] = false;
            return;
        }

        if (pressed_[key_index]) {
            return;
        }

        pressed_[key_index] = true;

        const auto modifiers = current_modifiers();

        if (key_event->key == start_hotkey_.key &&
            modifiers == start_hotkey_.modifiers) {
            enqueue_action(HotkeyAction::Start);
        } else if (key_event->key == stop_hotkey_.key &&
                   modifiers == stop_hotkey_.modifiers) {
            enqueue_action(HotkeyAction::Stop);
        } else if (key_event->key == configure_region_hotkey_.key &&
                   modifiers == configure_region_hotkey_.modifiers) {
            enqueue_action(HotkeyAction::ConfigureRegion);
        } else if (key_event->key == quit_key_.key &&
                   modifiers == quit_key_.modifiers) {
            enqueue_action(HotkeyAction::Quit);
        }
    }

    robot::EventTap& tap_;

    Hotkey start_hotkey_;
    Hotkey stop_hotkey_;
    Hotkey configure_region_hotkey_;
    Hotkey quit_key_;

    std::mutex mutex_;
    std::mutex lifecycle_mutex_;

    std::deque<HotkeyAction> actions_;
    std::array<bool, key_state_count> pressed_{};

    std::thread thread_;
    State state_{State::Stopped};
    bool stopping_{false};
};

app::HotkeyListener::HotkeyListener(
    RobotContext& context,
    const HotkeysConfig& config
)
    : impl_(std::make_unique<Impl>(
          context,
          config.start_fishing,
          config.stop_fishing,
          config.configure_region,
          config.quit))
{
}

app::HotkeyListener::~HotkeyListener() = default;

auto app::HotkeyListener::start() -> bool
{
    return impl_->start();
}

void app::HotkeyListener::stop() noexcept
{
    impl_->stop();
}

auto app::HotkeyListener::poll_action() noexcept
    -> std::optional<HotkeyAction>
{
    return impl_->poll_action();
}
