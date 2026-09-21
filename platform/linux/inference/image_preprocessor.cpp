#include "image_preprocessor.hpp"
#include <algorithm>
#include <cmath>

namespace degoonification::linux_backend {

ImagePreprocessor::ImagePreprocessor(int target_width, int target_height)
    : target_width_(target_width), target_height_(target_height) {}

LetterboxInfo ImagePreprocessor::preprocess(const VideoFrame& frame, std::vector<float>& out_planar_rgb) const {
    size_t channel_size = static_cast<size_t>(target_width_) * target_height_;
    out_planar_rgb.resize(3 * channel_size);

    LetterboxInfo info;
    info.orig_w = frame.width;
    info.orig_h = frame.height;

    if (!frame.data || frame.width == 0 || frame.height == 0) {
        std::fill(out_planar_rgb.begin(), out_planar_rgb.end(), 0.447f);
        return info;
    }

    // Aspect-ratio preserving letterbox scaling
    float scale = std::min(
        static_cast<float>(target_width_) / static_cast<float>(frame.width),
        static_cast<float>(target_height_) / static_cast<float>(frame.height)
    );
    int scaled_w = std::clamp(static_cast<int>(std::round(frame.width * scale)), 1, target_width_);
    int scaled_h = std::clamp(static_cast<int>(std::round(frame.height * scale)), 1, target_height_);
    int pad_x = (target_width_ - scaled_w) / 2;
    int pad_y = (target_height_ - scaled_h) / 2;

    info.scale = scale;
    info.pad_x = pad_x;
    info.pad_y = pad_y;
    info.scaled_w = scaled_w;
    info.scaled_h = scaled_h;

    // Fill with neutral gray (114/255 = 0.447f)
    std::fill(out_planar_rgb.begin(), out_planar_rgb.end(), 0.447f);

    float* r_plane = out_planar_rgb.data();
    float* g_plane = r_plane + channel_size;
    float* b_plane = g_plane + channel_size;

    const float norm_factor = 1.0f / 255.0f;
    const bool is_rgb24 = (frame.format == PixelFormat::RGB);
    const bool is_bgra = (frame.format == PixelFormat::BGRA || frame.format == PixelFormat::BGRx);
    const int bpp = is_rgb24 ? 3 : 4;
    const uint32_t stride = frame.stride > 0 ? frame.stride : (frame.width * bpp);

    const float inv_scale = 1.0f / scale;

    for (int y = 0; y < scaled_h; ++y) {
        int dest_y = pad_y + y;
        uint32_t src_y = std::min(static_cast<uint32_t>(y * inv_scale), frame.height - 1);
        const uint8_t* row_ptr = frame.data + (src_y * stride);
        size_t row_offset = dest_y * target_width_;

        for (int x = 0; x < scaled_w; ++x) {
            int dest_x = pad_x + x;
            uint32_t src_x = std::min(static_cast<uint32_t>(x * inv_scale), frame.width - 1);
            const uint8_t* px = row_ptr + (src_x * bpp);

            float c0 = px[0] * norm_factor;
            float c1 = px[1] * norm_factor;
            float c2 = px[2] * norm_factor;

            size_t idx = row_offset + dest_x;
            if (is_rgb24) {
                // px[0]=R, px[1]=G, px[2]=B
                r_plane[idx] = c0;
                g_plane[idx] = c1;
                b_plane[idx] = c2;
            } else if (is_bgra) {
                // px[0]=B, px[1]=G, px[2]=R
                r_plane[idx] = c2;
                g_plane[idx] = c1;
                b_plane[idx] = c0;
            } else {
                // px[0]=R, px[1]=G, px[2]=B (RGBA)
                r_plane[idx] = c0;
                g_plane[idx] = c1;
                b_plane[idx] = c2;
            }
        }
    }

    return info;
}

} // namespace degoonification::linux_backend
