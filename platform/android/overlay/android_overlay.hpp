#pragma once

#include "engine_shared/bounding_box.hpp"
#include <vector>
#include <cstdint>

namespace degoonification::android_backend {

struct AndroidOverlayConfig {
    float blur_radius{25.0f};
};

/**
 * Android SYSTEM_ALERT_WINDOW (TYPE_APPLICATION_OVERLAY) blur manager.
 * Interacts with Android RenderEffect.createBlurEffect() on API 31+.
 */
class AndroidOverlay {
public:
    explicit AndroidOverlay(AndroidOverlayConfig config = {});
    ~AndroidOverlay() = default;

    bool init();
    void render_boxes(const std::vector<core::BoundingBox>& boxes);

    [[nodiscard]] bool is_active() const noexcept { return is_active_; }

private:
    AndroidOverlayConfig config_;
    bool is_active_{false};
};

} // namespace degoonification::android_backend
