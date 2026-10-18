module;

#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

module app.config;

void app::validate(const AppConfig& config)
{
    if (config.model.path.empty()) {
        throw std::runtime_error{
            "Model path cannot be empty"
        };
    }

    if (config.model.input_size <= 0) {
        throw std::runtime_error{
            "Model input_size must be positive"
        };
    }

    if (config.model.confidence_threshold <= 0.0F ||
        config.model.confidence_threshold > 1.0F) {
        throw std::runtime_error{
            "Model confidence_threshold must be in range (0, 1]"
        };
    }

    if (config.model.path.empty()) {
        throw std::runtime_error(
            "The model path must not be empty."
        );
    }

    if (config.model.input_size <= 0) {
        throw std::runtime_error(
            "The model input size must be positive."
        );
    }

    if (!std::isfinite(config.model.confidence_threshold) ||
        config.model.confidence_threshold < 0.0F ||
        config.model.confidence_threshold > 1.0F) {
        throw std::runtime_error(
            "The confidence threshold must be between 0 and 1."
        );
    }

    if (config.model.backend.empty()) {
        throw std::runtime_error{
            "OpenCV backend path cannot be empty"
        };
    }

    if (config.capture.monitor_index < 0) {
        throw std::runtime_error{
            "Capture monitor_index cannot be negative"
        };
    }

    if (config.capture.width <= 0 ||
        config.capture.height <= 0) {
        throw std::runtime_error{
            "Capture region width and height must be positive"
        };
    }

    if (config.capture.fps_limit <= 0) {
        throw std::runtime_error{
            "Capture fps_limit must be positive"
        };
    }

    if (config.fishing.cast_cooldown_ms.count() < 0) {
        throw std::runtime_error{
            "Fishing cast_cooldown_ms cannot be negative"
        };
    }

    if (config.fishing.bobber_timeout_ms.count() <= 0) {
        throw std::runtime_error{
            "Fishing bobber_timeout_ms must be positive"
        };
    }

    if (config.fishing.splash_timeout_ms.count() <= 0) {
        throw std::runtime_error{
            "Fishing splash_timeout_ms must be positive"
        };
    }
}

auto app::load_config(
    const std::string& path
) -> AppConfig
{
    const auto root = YAML::LoadFile(path);

    AppConfig config;

    if (const auto model = root["model"]; model) {
        config.model.path =
            model["path"].as<std::string>(config.model.path);

        config.model.backend =
            model["backend"].as<std::string>(config.model.backend);

        config.model.input_size =
            model["input_size"].as<int>(config.model.input_size);

        config.model.confidence_threshold =
            model["confidence_threshold"].as<float>(
                config.model.confidence_threshold);

        if (const auto classes = model["classes"]; classes) {
            config.model.classes =
                classes.as<std::vector<std::string>>();
        }
    }

    if (const auto capture = root["capture"]; capture) {
        config.capture.monitor_index =
            capture["monitor_index"].as<int>(
                config.capture.monitor_index);

        config.capture.fps_limit =
            capture["fps_limit"].as<int>(
                config.capture.fps_limit);

        if (const auto region = capture["region"]; region) {
            config.capture.x =
                region["x"].as<int>(config.capture.x);

            config.capture.y =
                region["y"].as<int>(config.capture.y);

            config.capture.width =
                region["width"].as<int>(config.capture.width);

            config.capture.height =
                region["height"].as<int>(config.capture.height);
        }
    }

    if (const auto display = root["display"]; display) {
        config.display.show_preview =
            display["show_preview"].as<bool>(
                config.display.show_preview);

        config.display.show_confidence =
            display["show_confidence"].as<bool>(
                config.display.show_confidence);

        config.display.show_fps =
            display["show_fps"].as<bool>(
                config.display.show_fps);

        config.display.window_name =
            display["window_name"].as<std::string>(
                config.display.window_name);
    }

    if (const auto hotkeys = root["hotkeys"]; hotkeys) {
        config.hotkeys.start_fishing =
            hotkeys["start_fishing"].as<std::string>(
                config.hotkeys.start_fishing);

        config.hotkeys.stop_fishing =
            hotkeys["stop_fishing"].as<std::string>(
                config.hotkeys.stop_fishing);

        config.hotkeys.configure_region =
            hotkeys["configure_region"].as<std::string>(
                config.hotkeys.configure_region);

        config.hotkeys.quit =
            hotkeys["quit"].as<std::string>(
                config.hotkeys.quit);
    }

    if (const auto fishing = root["fishing"]; fishing) {
        config.fishing.cast_cooldown_ms =
            std::chrono::milliseconds{
                fishing["cast_cooldown_ms"].as<int>(
                    static_cast<int>(
                        config.fishing.cast_cooldown_ms.count()))};

        config.fishing.bobber_timeout_ms =
            std::chrono::milliseconds{
                fishing["bobber_timeout_ms"].as<int>(
                    static_cast<int>(
                        config.fishing.bobber_timeout_ms.count()))};

        config.fishing.splash_timeout_ms =
            std::chrono::milliseconds{
                fishing["splash_timeout_ms"].as<int>(
                    static_cast<int>(
                        config.fishing.splash_timeout_ms.count()))};

        config.fishing.cast_key =
            fishing["cast_key"].as<std::string>(
                config.fishing.cast_key);
    }

    validate(config);

    return config;
}
