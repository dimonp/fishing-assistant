module;

#include <array>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include <robot/Robot.h>

module app.key_parser;

namespace {

using app::Hotkey;
using app::ModifierMask;

struct KeyName {
    std::string_view name;
    robot::Key key;
};

struct ModifierName {
    std::string_view name;
    ModifierMask mask;
};

constexpr std::array modifier_names{
    ModifierName{"CTRL", app::modifier_ctrl},
    ModifierName{"CONTROL", app::modifier_ctrl},
    ModifierName{"SHIFT", app::modifier_shift},
    ModifierName{"ALT", app::modifier_alt},
    ModifierName{"OPTION", app::modifier_alt},
    ModifierName{"META", app::modifier_meta},
    ModifierName{"CMD", app::modifier_meta},
    ModifierName{"COMMAND", app::modifier_meta},
    ModifierName{"WIN", app::modifier_meta},
    ModifierName{"WINDOWS", app::modifier_meta},
    ModifierName{"SUPER", app::modifier_meta},
};

auto normalize(std::string_view text)-> std::string
{
    std::string result;
    result.reserve(text.size());

    for (const unsigned char ch : text) {
        if (std::isspace(ch) != 0) {
            continue;
        }

        result.push_back(
            static_cast<char>(std::toupper(ch)));
    }

    return result;
}

auto find_modifier(std::string_view name)-> ModifierMask
{
    for (const auto& entry : modifier_names) {
        if (entry.name == name) {
            return entry.mask;
        }
    }

    return 0;
}

auto named_keys()
    -> const std::unordered_map<std::string_view, robot::Key>&
{
    using enum robot::Key;

    static const std::unordered_map<std::string_view, robot::Key>
        keys{
            {"SPACE", Space},
            {"ENTER", Enter},
            {"RETURN", Enter},
            {"ESC", Escape},
            {"ESCAPE", Escape},
            {"TAB", Tab},
            {"BACKSPACE", Backspace},
            {"BACK", Backspace},
            {"DELETE", Delete},
            {"DEL", Delete},
            {"INSERT", Insert},
            {"INS", Insert},
            {"HOME", Home},
            {"END", End},
            {"PAGEUP", PageUp},
            {"PGUP", PageUp},
            {"PAGEDOWN", PageDown},
            {"PGDN", PageDown},

            {"LEFT", LeftArrow},
            {"RIGHT", RightArrow},
            {"UP", UpArrow},
            {"DOWN", DownArrow},

            {"CAPSLOCK", CapsLock},
            {"NUMLOCK", NumLock},
            {"SCROLLLOCK", ScrollLock},
            {"PRINTSCREEN", PrintScreen},
            {"PRINT", PrintScreen},
            {"PAUSE", Pause},
            {"BREAK", Pause},

            {"LEFTCTRL", LeftControl},
            {"RIGHTCTRL", RightControl},
            {"LEFTCONTROL", LeftControl},
            {"RIGHTCONTROL", RightControl},
            {"LCTRL", LeftControl},
            {"RCTRL", RightControl},

            {"LEFTSHIFT", LeftShift},
            {"RIGHTSHIFT", RightShift},
            {"LSHIFT", LeftShift},
            {"RSHIFT", RightShift},

            {"LEFTALT", LeftAlt},
            {"RIGHTALT", RightAlt},
            {"LALT", LeftAlt},
            {"RALT", RightAlt},

            {"LEFTMETA", LeftMeta},
            {"RIGHTMETA", RightMeta},
            {"LMETA", LeftMeta},
            {"RMETA", RightMeta},

            {"NUMPAD0", Keypad0},
            {"NUMPAD1", Keypad1},
            {"NUMPAD2", Keypad2},
            {"NUMPAD3", Keypad3},
            {"NUMPAD4", Keypad4},
            {"NUMPAD5", Keypad5},
            {"NUMPAD6", Keypad6},
            {"NUMPAD7", Keypad7},
            {"NUMPAD8", Keypad8},
            {"NUMPAD9", Keypad9},

            {"NUMPADADD", KeypadPlus},
            {"NUMPADSUBTRACT", KeypadMinus},
            {"NUMPADMULTIPLY", KeypadMultiply},
            {"NUMPADDIVIDE", KeypadDivide},
            {"NUMPADDECIMAL", KeypadDecimal},
            {"NUMPADENTER", KeypadEnter},
        };

    return keys;
}

auto parse_normalized_key(std::string_view name)-> robot::Key
{
    using enum robot::Key;

    if (name.empty()) {
        throw std::invalid_argument{"Key name cannot be empty"};
    }

    if (name.size() == 1) {
        const char ch = name.front();

        if (ch >= 'A' && ch <= 'Z') {
            return static_cast<robot::Key>(
                static_cast<int>(A) + (ch - 'A'));
        }

        if (ch >= '0' && ch <= '9') {
            switch (ch) {
            case '0': return Digit0;
            case '1': return Digit1;
            case '2': return Digit2;
            case '3': return Digit3;
            case '4': return Digit4;
            case '5': return Digit5;
            case '6': return Digit6;
            case '7': return Digit7;
            case '8': return Digit8;
            case '9': return Digit9;
            default: break;
            }
        }
    }

    if (name.size() >= 2 && name.front() == 'F') {
        unsigned int number = 0;

        for (std::size_t i = 1; i < name.size(); ++i) {
            const char ch = name[i];

            if (ch < '0' || ch > '9') {
                throw std::invalid_argument{
                    "Invalid function key: " + std::string{name}};
            }

            number = number * 10U
                   + static_cast<unsigned int>(ch - '0');
        }

        if (number >= 1 && number <= 24) {
            switch (number) {
            case 1:  return F1;
            case 2:  return F2;
            case 3:  return F3;
            case 4:  return F4;
            case 5:  return F5;
            case 6:  return F6;
            case 7:  return F7;
            case 8:  return F8;
            case 9:  return F9;
            case 10: return F10;
            case 11: return F11;
            case 12: return F12;
            case 13: return F13;
            case 14: return F14;
            case 15: return F15;
            case 16: return F16;
            case 17: return F17;
            case 18: return F18;
            case 19: return F19;
            case 20: return F20;
            case 21: return F21;
            case 22: return F22;
            case 23: return F23;
            case 24: return F24;
            default: break;
            }
        }

        throw std::invalid_argument{
            "Function key must be between F1 and F24"};
    }

    const auto& keys = named_keys();
    const auto it = keys.find(name);

    if (it != keys.end()) {
        return it->second;
    }

    throw std::invalid_argument{
        "Unknown key name: " + std::string{name}};
}

} // namespace

namespace app {

auto parse_key(std::string_view name)-> robot::Key
{
    const auto normalized = normalize(name);
    return parse_normalized_key(normalized);
}

auto parse_hotkey(std::string_view text) -> Hotkey
{
    if (text.empty()) {
        throw std::invalid_argument{
            "Hotkey cannot be empty"};
    }

    ModifierMask modifiers = 0;
    bool has_key = false;
    std::string key_name;

    std::size_t begin = 0;

    while (begin <= text.size()) {
        const auto separator = text.find('+', begin);
        const auto end = separator == std::string_view::npos
            ? text.size()
            : separator;

        const auto token = text.substr(begin, end - begin);
        const auto normalized = normalize(token);

        if (normalized.empty()) {
            throw std::invalid_argument{
                "Hotkey contains an empty component"};
        }

        const ModifierMask modifier = find_modifier(normalized);

        if (modifier != 0) {
            if (has_key) {
                throw std::invalid_argument{
                    "Modifier cannot appear after the key"};
            }

            if ((modifiers & modifier) != 0) {
                throw std::invalid_argument{
                    "Duplicate modifier in hotkey"};
            }

            modifiers |= modifier;
        } else {
            if (has_key) {
                throw std::invalid_argument{
                    "Hotkey must contain exactly one non-modifier key"};
            }

            key_name = normalized;
            has_key = true;
        }

        if (separator == std::string_view::npos) {
            break;
        }

        begin = separator + 1;
    }

    if (!has_key) {
        throw std::invalid_argument{
            "Hotkey must contain a key"};
    }

    return Hotkey{
        .key = parse_normalized_key(key_name),
        .modifiers = modifiers,
    };
}

} // namespace app
