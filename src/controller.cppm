module;

#include <chrono>
#include <thread>
#include <utility>
#include <robot/Robot.h>

export module app.controller;

import app.robot_context;

export namespace app {

using namespace std::chrono_literals;

class Controller {
public:
    explicit Controller(RobotContext& context) noexcept
        : mouse_(context.session().mouse())
        , keyboard_(context.session().keyboard())
    {}

    void move(double x, double y)
    {
        std::ignore = mouse_.move({x, y});
    }

    void move_smooth(double x, double y)
    {
        std::ignore = mouse_.moveSmooth(
            {x, y},
            robot::MouseMoveOptions{500ms, 5});
    }

    void click_left()
    {
        std::ignore = mouse_.click();
    }

    void click_right()
    {
        std::ignore = mouse_.click(robot::MouseButton::Right);
    }

    void double_click_left(
        std::chrono::milliseconds interval = 120ms)
    {
        click_left();
        std::this_thread::sleep_for(interval);
        click_left();
    }

    void double_click_right(
        std::chrono::milliseconds interval = 120ms)
    {
        click_right();
        std::this_thread::sleep_for(interval);
        click_right();
    }

    void position()
    {
        std::ignore = mouse_.position();
    }

    void tap_key(
        robot::Key key,
        robot::Modifiers modifiers = {})
    {
        std::ignore = keyboard_.tap(key, modifiers);
    }

    void key_down(robot::Key key)
    {
        std::ignore = keyboard_.press(key);
    }

    void key_up(robot::Key key)
    {
        std::ignore = keyboard_.release(key);
    }

private:
    robot::Mouse& mouse_;
    robot::Keyboard& keyboard_;
};

} // namespace app