#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "engine_shared/bounding_box.hpp"
#include <vector>
#include <cstdint>

namespace degoonification::windows_backend {

struct WindowsOverlayConfig {
    uint32_t frosted_color_argb{0xEE111827};
    bool clickthrough{true};
};

/**
 * Win32 Layered Window Overlay.
 * Uses WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST for 100% click-through
 * and hardware-accelerated Direct2D / GDI blur rendering.
 */
class WindowsOverlay {
public:
    explicit WindowsOverlay(WindowsOverlayConfig config = {});
    ~WindowsOverlay();

    bool init();
    void render_boxes(const std::vector<core::BoundingBox>& boxes);
    void message_pump();

    [[nodiscard]] bool is_active() const noexcept { return is_active_; }

private:
#ifdef _WIN32
    HWND hwnd_{nullptr};
    HDC mem_dc_{nullptr};
    HBITMAP mem_bitmap_{nullptr};
    void* bitmap_pixels_{nullptr};
#endif
    WindowsOverlayConfig config_;
    bool is_active_{false};
    int screen_width_{1920};
    int screen_height_{1080};
};

} // namespace degoonification::windows_backend
