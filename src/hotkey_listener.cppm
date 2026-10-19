module;

#include <memory>
#include <optional>

export module app.hotkey_listener;

import app.robot_context;
import app.config;

export namespace app {

enum class HotkeyAction {
    Start,
    Stop,
    ConfigureRegion,
    Quit
};

class HotkeyListener {
public:
    HotkeyListener(
        RobotContext& context,
        const HotkeysConfig& config);

    ~HotkeyListener();

    HotkeyListener(const HotkeyListener&) = delete;
    auto operator=(const HotkeyListener&) -> HotkeyListener& = delete;

    [[nodiscard]]
    auto start() -> bool;
    void stop() noexcept;

    [[nodiscard]]
    auto poll_action() noexcept
        -> std::optional<HotkeyAction>;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace app
