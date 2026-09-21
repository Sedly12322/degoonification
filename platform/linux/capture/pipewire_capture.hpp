#pragma once

#include "video_frame.hpp"
#include <functional>
#include <memory>
#include <atomic>
#include <cstdint>

namespace degoonification::linux_backend {

using FrameCallback = std::function<void(const VideoFrame&)>;

struct PipeWireConfig {
    uint32_t target_node_id{0}; // 0 = default/auto
    uint32_t target_fps{15};     // 15 FPS default for power efficiency
    bool request_dmabuf{true};
};

/**
 * PipeWire ScreenCast Stream Client.
 * Connects to a PipeWire video stream (provided by xdg-desktop-portal ScreenCast)
 * and receives video frames with DMA-BUF zero-copy support.
 */
class PipeWireCapture {
public:
    explicit PipeWireCapture(PipeWireConfig config = {});
    ~PipeWireCapture();

    PipeWireCapture(const PipeWireCapture&) = delete;
    PipeWireCapture& operator=(const PipeWireCapture&) = delete;

    /**
     * Initializes the PipeWire loop and thread.
     */
    bool init();

    /**
     * Connects to the specified PipeWire stream fd / node.
     */
    bool start(int stream_fd, FrameCallback callback);

    /**
     * Stops the stream and disconnects.
     */
    void stop();

    [[nodiscard]] bool is_running() const noexcept;
    [[nodiscard]] uint32_t current_width() const noexcept;
    [[nodiscard]] uint32_t current_height() const noexcept;

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
    PipeWireConfig config_;
};

} // namespace degoonification::linux_backend
