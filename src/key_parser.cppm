module;

#include <cstdint>
#include <string_view>
#include <robot/Robot.h>

export module app.key_parser;

export namespace app {

using ModifierMask = std::uint8_t;

inline constexpr ModifierMask modifier_ctrl  = 1U << 0;
inline constexpr ModifierMask modifier_shift = 1U << 1;
inline constexpr ModifierMask modifier_alt   = 1U << 2;
inline constexpr ModifierMask modifier_meta  = 1U << 3;

struct Hotkey {
    robot::Key key{robot::Key::Unknown};
    ModifierMask modifiers{};
};

[[nodiscard]]
auto parse_key(std::string_view name)
    -> robot::Key;

[[nodiscard]]
auto parse_hotkey(std::string_view text)
    -> Hotkey;

} // namespace app