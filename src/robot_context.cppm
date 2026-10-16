
module;

#include <memory>
#include <expected>

#include <robot/Robot.h>

export module app.robot_context;

export namespace app {

class RobotContext {
public:
    [[nodiscard]] static auto create()
        -> std::expected<RobotContext, robot::Error>;

    RobotContext(RobotContext&&) noexcept = default;
    auto operator=(RobotContext&&) noexcept -> RobotContext& = default;

    RobotContext(const RobotContext&) = delete;
    auto operator=(const RobotContext&) -> RobotContext& = delete;

    [[nodiscard]] auto session() noexcept -> robot::Session& {
        return *session_;
    }

private:
    explicit RobotContext(std::unique_ptr<robot::Session> session)
        : session_(std::move(session)) {}

    std::unique_ptr<robot::Session> session_;
};

} // namespace app
