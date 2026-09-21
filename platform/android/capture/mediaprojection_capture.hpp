#pragma once

#include <functional>
#include <cstdint>

namespace degoonification::android_backend {

struct AndroidHardwareFrame {
    uint32_t width{0};
    uint32_t height{0};
    uint32_t stride{0};
    void* hardware_buffer{nullptr}; // AHardwareBuffer*
    uint64_t timestamp_ns{0};
};

using AndroidFrameCallback = std::function<void(const AndroidHardwareFrame&)>;

/**
 * Android MediaProjection with AHardwareBuffer.
 * Implements Dynamic Motion Heuristic: 1 FPS when idle, up to 12 FPS during scrolling/motion.
 */
class MediaProjectionCapture {
public:
    MediaProjectionCapture() = default;
    ~MediaProjectionCapture() = default;

    bool init();
    bool start(AndroidFrameCallback callback);
    void stop();

    void set_dynamic_fps(uint32_t fps);

    [[nodiscard]] bool is_active() const noexcept { return is_active_; }

private:
    bool is_active_{false};
    uint32_t current_fps_{12};
    AndroidFrameCallback callback_;
};

} // namespace degoonification::android_backend
