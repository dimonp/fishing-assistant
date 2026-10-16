export module app.region_selector;

import app.screen_capture;

export namespace app
{

[[nodiscard]] auto select_capture_region(
    ScreenCapture& screen
) -> CaptureRegion;

} // namespace app