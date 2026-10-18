module;

#include <memory>

export module app.display_manager;

import app.robot_context;

export namespace app {

struct PhysicalPoint {
    int x{};
    int y{};
};

struct LogicalPoint {
    double x{};
    double y{};
};

struct DisplayInfo {
    int index{};

    int physical_x{};
    int physical_y{};
    int physical_width{};
    int physical_height{};

    double logical_x{};
    double logical_y{};
    double scale_factor{1.0};
};

class DisplayManager {
public:
    DisplayManager(RobotContext& context, int monitor_index);
    ~DisplayManager();

    DisplayManager(const DisplayManager&) = delete;
    auto operator=(const DisplayManager&) -> DisplayManager& = delete;

    DisplayManager(DisplayManager&&) noexcept;
    auto operator=(DisplayManager&&) noexcept -> DisplayManager&;

    [[nodiscard]]
    auto display_info() const -> DisplayInfo;

    [[nodiscard]]
    auto physical_to_logical(PhysicalPoint point) const -> LogicalPoint;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace app