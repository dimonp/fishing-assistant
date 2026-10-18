module;

#include <expected>
#include <memory>
#include <string>
#include <vector>

export module app.fishing_action;

import app.display_manager;
import app.controller;
import app.config;
import app.detector;

export namespace app {

class FishingAction {
public:
    FishingAction(
        Controller& controller,
        const DisplayManager& display,
        const FishingConfig& config
    );

    ~FishingAction();

    FishingAction(const FishingAction&) = delete;
    auto operator=(const FishingAction&) -> FishingAction& = delete;

    FishingAction(FishingAction&&) noexcept;
    auto operator=(FishingAction&&) noexcept -> FishingAction&;

    void set_region_origin(int x, int y) noexcept;

    [[nodiscard]]
    auto process(
        const std::vector<Detection>& detections
    ) -> std::expected<void, std::string>;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace app