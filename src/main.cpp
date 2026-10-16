#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/core/utils/logger.hpp>

import app.config;
import app.detector;
import app.hotkeys;
import app.display;
import app.robot_context;
import app.screen_capture;
import app.region_selector;
import app.fishing_action;
import app.controller;

namespace
{

[[nodiscard]] auto get_class_name(
    const std::vector<std::string>& classes,
    const int class_id
) -> std::string
{
    if (class_id < 0 ||
        static_cast<std::size_t>(class_id) >= classes.size()) {
        return "class_" + std::to_string(class_id);
    }

    return classes[static_cast<std::size_t>(class_id)];
}

void draw_detections(
    cv::Mat& frame,
    const std::vector<app::Detection>& detections,
    const std::vector<std::string>& classes,
    const bool show_confidence
)
{
    for (const auto& detection : detections) {
        const cv::Rect box{
            static_cast<int>(std::lround(detection.box.x)),
            static_cast<int>(std::lround(detection.box.y)),
            static_cast<int>(std::lround(detection.box.width)),
            static_cast<int>(std::lround(detection.box.height))
        };

        cv::rectangle(frame, box, cv::Scalar{0, 255, 0}, 2);

        std::string label =
            get_class_name(classes, detection.class_id);

        if (show_confidence) {
            label += " " + std::to_string(detection.confidence).substr(0, 4);
        }

        const int text_y = std::max(20, box.y - 8);

        cv::putText(
            frame,
            label,
            {box.x, text_y},
            cv::FONT_HERSHEY_SIMPLEX,
            0.6,
            cv::Scalar{0, 255, 0},
            2,
            cv::LINE_AA
        );
    }
}

void handle_hotkeys(
    app::HotkeyListener& hotkeys,
    app::ScreenCapture& screen,
    app::FishingAction& fishing,
    bool& running,
    bool& quit,
    std::chrono::steady_clock::time_point& next_frame
)
{
    while (const auto action = hotkeys.poll_action()) {
        switch (*action) {
        case app::HotkeyAction::Start:
            running = true;
            next_frame = std::chrono::steady_clock::now();
            std::cout << "Fishing started.\n";
            break;

        case app::HotkeyAction::Stop:
            running = false;
            std::cout << "Fishing stopped.\n";
            break;

        case app::HotkeyAction::ConfigureRegion: {
            const bool was_running = running;
            running = false;

            try {

                const auto selected =
                    app::select_capture_region(screen);

                if (selected.width > 0 && selected.height > 0) {
                    screen.set_region(selected);
                    fishing.set_region_origin(selected.x, selected.y);

                    std::cout << "Capture region updated: "
                              << selected.x << ", "
                              << selected.y << ", "
                              << selected.width << "x"
                              << selected.height << '\n';
                } else {
                    std::cout << "Region selection cancelled; "
                                 "previous region retained.\n";
                }
            } catch (const std::exception& error) {
                std::cerr << "Failed to configure capture region: "
                          << error.what() << '\n';
            }

            running = was_running;
            next_frame = std::chrono::steady_clock::now();
            break;
        }

        case app::HotkeyAction::Quit:
            quit = true;
            break;
        }
    }
}

[[nodiscard]] auto process_frame(
    app::ScreenCapture& screen,
    app::Detector& detector,
    app::FishingAction& fishing_action,
    const app::AppConfig& config,
    double fps
) -> bool
{
    auto frame = screen.capture();

    if (frame.empty()) {
        std::cerr << "Screen capture failed; processing stopped.\n";
        return false;
    }

    auto detections_result = detector.detect(frame);

    if (!detections_result) {
        std::cerr << "Detection failed: "
                  << detections_result.error().message << '\n';
        return false;
    }

    if (config.display.show_preview) {
        draw_detections(
            frame,
            *detections_result,
            config.model.classes,
            config.display.show_confidence
        );

        if (config.display.show_fps) {
            const std::string fps_label =
                "FPS: " + std::to_string(static_cast<int>(std::lround(fps)));

            cv::putText(
                frame,
                fps_label,
                {10, 30},
                cv::FONT_HERSHEY_SIMPLEX,
                0.8,
                cv::Scalar{0, 255, 255},
                2,
                cv::LINE_AA
            );
        }

        cv::imshow(config.display.window_name, frame);
    }

    auto fishing_result = fishing_action.process(*detections_result);

    if (!fishing_result) {
        std::cerr << "Fishing action failed: "
                  << fishing_result.error() << '\n';
    }

    return true;
}

[[nodiscard]] auto initialize_detector(
    app::Detector& detector,
    const auto& config
) -> bool
{
    auto result = detector.load(config.model);

    if (!result) {
        std::cerr << "Failed to load detector: "
                  << result.error().message << '\n';
        return false;
    }

    return true;
}

[[nodiscard]] auto run_application(
    const auto& config,
    app::RobotContext& context
) -> int
{
    app::DisplayManager display{context, config.capture.monitor_index};
    app::ScreenCapture screen{context, display};

    screen.set_region({
        config.capture.x,
        config.capture.y,
        config.capture.width,
        config.capture.height
    });

    app::Detector detector;

    if (!initialize_detector(detector, config)) {
        return 1;
    }

    app::HotkeyListener hotkeys{
        context,
        config.hotkeys,
    };

    if (!hotkeys.start()) {
        std::cerr << "Failed to start hotkey listener.\n";
        return 1;
    }

    app::Controller mouse{context};
    app::FishingAction fishing{mouse, display, config.fishing};

    fishing.set_region_origin(config.capture.x, config.capture.y);

    bool running = false;
    bool quit = false;

    const auto frame_interval =
        config.capture.fps_limit > 0
            ? std::chrono::duration_cast<
                  std::chrono::steady_clock::duration>(
                  std::chrono::duration<double>(
                      1.0 / config.capture.fps_limit))
            : std::chrono::steady_clock::duration::zero();

    auto next_frame = std::chrono::steady_clock::now();

    std::cout << "Ready. Press "
              << config.hotkeys.start_fishing << " to start fishing, "
              << config.hotkeys.stop_fishing << " to stop fishing, "
              << config.hotkeys.configure_region
              << " to configure the capture region, "
              << config.hotkeys.quit << " to quit.\n";

    if (config.display.show_preview) {
        cv::namedWindow(config.display.window_name, cv::WINDOW_NORMAL);
    }

    auto previous_frame_time = std::chrono::steady_clock::now();
    double current_fps = 0.0;

    while (!quit) {

        if (quit) {
            break;
        }

        handle_hotkeys(
            hotkeys,
            screen,
            fishing,
            running,
            quit,
            next_frame
        );

        if (config.display.show_preview) {
            cv::waitKey(1);
        }

        if (!running) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds{10}
            );
            continue;
        }

        const auto now = std::chrono::steady_clock::now();

        if (frame_interval >
                std::chrono::steady_clock::duration::zero() &&
            now < next_frame) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds{1}
            );
            continue;
        }

        const auto frame_start = std::chrono::steady_clock::now();

        const auto elapsed =
            std::chrono::duration<double>(
                frame_start - previous_frame_time
            ).count();

            if (elapsed > 0.0) {
                current_fps = 1.0 / elapsed;
            }

            previous_frame_time = frame_start;

            if (!process_frame(
                screen,
                detector,
                fishing,
                config,
                current_fps
            )) {
                running = false;
                continue;
            }

        if (frame_interval >
            std::chrono::steady_clock::duration::zero()) {
            next_frame =
                std::chrono::steady_clock::now() + frame_interval;
        }
    }

    hotkeys.stop();
    cv::destroyAllWindows();

    return 0;
}

} // namespace

auto main() -> int
{
    try {
        const auto config = app::load_config("config.yaml");

        auto context_result = app::RobotContext::create();

        if (!context_result) {
            std::cerr << "Failed to initialize robot-cpp: "
                      << context_result.error().message << '\n';
            return 1;
        }

        auto& context = *context_result;

        if (!context.session().capabilities().canCaptureScreen) {
            std::cerr << "Screen capture is not supported "
                         "by the current robot-cpp backend.\n";
            return 1;
        }

       cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

        return run_application(config, context);
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        cv::destroyAllWindows();
        return 1;
    }
}