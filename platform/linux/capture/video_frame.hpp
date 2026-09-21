#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace degoonification::linux_backend {

enum class PixelFormat {
    RGBA,
    BGRA,
    RGBx,
    BGRx,
    UNKNOWN
};

struct VideoFrame {
    uint32_t width{0};
    uint32_t height{0};
    uint32_t stride{0};
    PixelFormat format{PixelFormat::BGRA};
    uint64_t timestamp_us{0};

    /// Linux DMA-BUF file descriptor (-1 if software memory buffer)
    int dmabuf_fd{-1};

    /// CPU mapped pointer (valid when mapped or software fallback)
    const uint8_t* data{nullptr};
    size_t data_size{0};
};

} // namespace degoonification::linux_backend
