#pragma once

#include "engine_shared/bounding_box.hpp"
#include "../capture/video_frame.hpp"
#include <vector>
#include <memory>
#include <cstdint>

namespace degoonification::linux_backend {

struct OverlayConfig {
    uint32_t frosted_color_argb{0xFF0F172A}; // fallback opaque dark slate
    bool enable_clickthrough{true};
    bool enable_frosted_mosaic{true}; // Frosted mosaic blur from underlying screen pixels
    int mosaic_block_size{18};       // Size in pixels of mosaic censorship blocks
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
     * When source_frame is provided, applies real frosted mosaic blur using
     * the captured screen pixels.
     * When boxes is empty, clears the surface to 100% transparent.
     */
    void render_boxes(
        const std::vector<core::BoundingBox>& boxes,
        const VideoFrame* source_frame = nullptr
    );

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
