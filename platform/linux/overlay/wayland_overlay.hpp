#pragma once

#include "engine_shared/bounding_box.hpp"
#include <vector>
#include <memory>
#include <cstdint>

namespace degoonification::linux_backend {

struct OverlayConfig {
    uint32_t frosted_color_argb{0xEE111827}; // Dark frosted privacy blur fill
    bool enable_clickthrough{true};
};

/**
 * High-performance Wayland click-through Overlay using wlr-layer-shell protocol.
 * Spans the entire screen on the OVERLAY layer with an empty input region,
 * ensuring mouse and keyboard events pass through seamlessly to underlying apps.
 */
class WaylandOverlay {
public:
    explicit WaylandOverlay(OverlayConfig config = {});
    ~WaylandOverlay();

    WaylandOverlay(const WaylandOverlay&) = delete;
    WaylandOverlay& operator=(const WaylandOverlay&) = delete;

    /**
     * Connects to Wayland compositor and initializes the layer surface.
     */
    bool init();

    /**
     * Renders explicit content blur boxes onto the overlay surface.
     * When boxes is empty, clears the surface to 100% transparent.
     */
    void render_boxes(const std::vector<core::BoundingBox>& boxes);

    /**
     * Dispatches pending Wayland event queue non-blockingly.
     */
    void dispatch_events();

    [[nodiscard]] bool is_ready() const noexcept;
    [[nodiscard]] uint32_t screen_width() const noexcept;
    [[nodiscard]] uint32_t screen_height() const noexcept;

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
    OverlayConfig config_;
};

} // namespace degoonification::linux_backend
