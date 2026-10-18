module;

#include <cstddef>
#include <memory>
#include <stdexcept>

#include <robot/Robot.h>

module app.display_manager;

namespace app
{

class DisplayManager::Impl
{
public:
    Impl(RobotContext& context, int monitor_index)
        : session{context.session()}
        , monitor_index_{monitor_index}
    {
        if (monitor_index_ < 0) {
            throw std::invalid_argument{
                "Monitor index cannot be negative"
            };
        }

        // Проверяем, что монитор существует при создании.
        (void)get_display_info();
    }

    [[nodiscard]]
    auto get_monitor() const -> robot::Monitor
    {
        const auto monitors = session.screen().monitors();

        if (!monitors) {
            throw std::runtime_error{
                "Failed to enumerate monitors"
            };
        }

        if (static_cast<std::size_t>(monitor_index_) >= monitors->size()) {
            throw std::out_of_range{
                "Monitor index is out of range"
            };
        }

        return (*monitors)[static_cast<std::size_t>(monitor_index_)];
    }

    [[nodiscard]]
    auto get_display_info() const -> DisplayInfo
    {
        const auto monitor = get_monitor();

        const double scale = monitor.scaleFactor;

        if (scale <= 0.0) {
            throw std::runtime_error{
                "Invalid monitor scale factor"
            };
        }

        const auto& physical = monitor.physicalBounds;
        const auto& logical = monitor.logicalBounds;

        return DisplayInfo{
            .index = monitor_index_,

            .physical_x = physical.origin.x,
            .physical_y = physical.origin.y,
            .physical_width = physical.size.width,
            .physical_height = physical.size.height,

            .logical_x = logical.origin.x,
            .logical_y = logical.origin.y,
            .scale_factor = scale
        };
    }

    [[nodiscard]]
    auto physical_to_logical(const PhysicalPoint& point) const -> LogicalPoint
    {
        const auto display = get_display_info();

        return {
            display.logical_x +
                static_cast<double>(point.x) / display.scale_factor,

            display.logical_y +
                static_cast<double>(point.y) / display.scale_factor
        };
    }

    robot::Session& session;
    int monitor_index_;
};

DisplayManager::DisplayManager(
    RobotContext& context,
    int monitor_index
)
    : impl_{std::make_unique<Impl>(context, monitor_index)}
{
}

DisplayManager::~DisplayManager() = default;

DisplayManager::DisplayManager(DisplayManager&&) noexcept = default;

auto DisplayManager::operator=(DisplayManager&&) noexcept
    -> DisplayManager& = default;

auto DisplayManager::display_info() const -> DisplayInfo
{
    return impl_->get_display_info();
}

auto DisplayManager::physical_to_logical(
    PhysicalPoint point
) const -> LogicalPoint
{
    return impl_->physical_to_logical(point);
}

} // namespace app