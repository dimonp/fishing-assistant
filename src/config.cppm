module;

#include <string>
#include <vector>
#include <chrono>

#include <yaml-cpp/yaml.h>

export module app.config;

export namespace app {

    struct DisplayConfig {
        bool show_preview = true;
        bool show_confidence = true;
        bool show_fps = true;
        std::string window_name = "Detection Preview";
    };

    struct HotkeysConfig {
        std::string start_fishing{"F6"};
        std::string stop_fishing{"F7"};
        std::string configure_region{"F8"};
        std::string quit{"Esc"};
    };

    struct ModelConfig {
        std::string path;
        std::string backend{"cpu"};
        int input_size{640};
        float confidence_threshold{0.6F};
        std::vector<std::string> classes;
    };

    struct CaptureConfig {
        int monitor_index{0};
        int x{};
        int y{};
        int width{800};
        int height{600};
        int fps_limit{15};
    };

    struct FishingConfig {
        std::chrono::milliseconds cast_cooldown_ms{1500};
        std::chrono::milliseconds bobber_timeout_ms{5000};
        std::chrono::milliseconds splash_timeout_ms{30000};
        std::string cast_key{};
    };

    struct AppConfig {
        ModelConfig model;
        DisplayConfig display;
        HotkeysConfig hotkeys;
        CaptureConfig capture;
        FishingConfig fishing;
    };

    [[nodiscard]] auto load_config(
        const std::string& path
    ) -> AppConfig;

    void validate(const AppConfig& config);

} // namespace app
