#pragma once

#include "video_frame.hpp"
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace degoonification::linux_backend {

/**
 * High-performance Wayland screencopy capture engine using grim / zwlr_screencopy.
 * Captures fullscreen frames into RAM (/dev/shm) via direct compositor access,
 * providing zero-permission-prompt, sub-25ms screen frame streaming on Hyprland/Sway.
 */
class ScreencopyCapture {
public:
    ScreencopyCapture();
    ~ScreencopyCapture();

    ScreencopyCapture(const ScreencopyCapture&) = delete;
    ScreencopyCapture& operator=(const ScreencopyCapture&) = delete;

    /**
     * Checks if grim capture utility is available in system PATH.
     */
    static bool is_available() noexcept;

    /**
     * Captures a single frame of the current active screen.
     * Returns true on success, filling frame metadata and raw RGB24 buffer pointer.
     */
    bool capture_frame(VideoFrame& out_frame);

    [[nodiscard]] uint32_t last_width() const noexcept { return last_width_; }
    [[nodiscard]] uint32_t last_height() const noexcept { return last_height_; }

private:
    std::vector<uint8_t> buffer_;
    std::string shm_path_;
    uint32_t last_width_{0};
    uint32_t last_height_{0};
};

} // namespace degoonification::linux_backend
