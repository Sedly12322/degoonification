#include "image_preprocessor.hpp"
#include <algorithm>

namespace degoonification::linux_backend {

ImagePreprocessor::ImagePreprocessor(int target_width, int target_height)
    : target_width_(target_width), target_height_(target_height) {}

void ImagePreprocessor::preprocess(const VideoFrame& frame, std::vector<float>& out_planar_rgb) const {
    size_t channel_size = static_cast<size_t>(target_width_) * target_height_;
    out_planar_rgb.resize(3 * channel_size);

    if (!frame.data || frame.width == 0 || frame.height == 0) {
        std::fill(out_planar_rgb.begin(), out_planar_rgb.end(), 0.0f);
        return;
    }

    float* r_plane = out_planar_rgb.data();
    float* g_plane = r_plane + channel_size;
    float* b_plane = g_plane + channel_size;

    const float scale_x = static_cast<float>(frame.width) / static_cast<float>(target_width_);
    const float scale_y = static_cast<float>(frame.height) / static_cast<float>(target_height_);
    const float norm_factor = 1.0f / 255.0f;

    const bool is_rgb24 = (frame.format == PixelFormat::RGB);
    const bool is_bgra = (frame.format == PixelFormat::BGRA || frame.format == PixelFormat::BGRx);
    const int bpp = is_rgb24 ? 3 : 4;
    const uint32_t stride = frame.stride > 0 ? frame.stride : (frame.width * bpp);

    for (int y = 0; y < target_height_; ++y) {
        uint32_t src_y = std::min(static_cast<uint32_t>(y * scale_y), frame.height - 1);
        const uint8_t* row_ptr = frame.data + (src_y * stride);
        size_t row_offset = y * target_width_;

        for (int x = 0; x < target_width_; ++x) {
            uint32_t src_x = std::min(static_cast<uint32_t>(x * scale_x), frame.width - 1);
            const uint8_t* px = row_ptr + (src_x * bpp);

            float c0 = px[0] * norm_factor;
            float c1 = px[1] * norm_factor;
            float c2 = px[2] * norm_factor;

            size_t idx = row_offset + x;
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
}

} // namespace degoonification::linux_backend
