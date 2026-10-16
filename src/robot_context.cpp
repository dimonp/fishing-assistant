
module;

#include <memory>
#include <utility>

#include <robot/Robot.h>

module app.robot_context;

auto app::RobotContext::create()
    -> std::expected<RobotContext, robot::Error>
{
    auto result = robot::Session::create();

    if (!result) {
        return std::unexpected(result.error());
    }

    return RobotContext{std::move(*result)};
}
