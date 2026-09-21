#pragma once

#include "../capture/video_frame.hpp"
#include <vector>
#include <cstdint>

namespace degoonification::linux_backend {

struct LetterboxInfo {
    float scale{1.0f};
    int pad_x{0};
    int pad_y{0};
    int scaled_w{640};
    int scaled_h{640};
    uint32_t orig_w{0};
    uint32_t orig_h{0};
};

/**
 * High-performance frame preprocessor.
 * Converts captured screen pixels (BGRA/RGBA) into planar float32 RGB [3 x 640 x 640]
 * normalized to [0.0, 1.0] with aspect-ratio preserving letterboxing for YOLOv8 ONNX.
 */
class ImagePreprocessor {
public:
    ImagePreprocessor(int target_width = 640, int target_height = 640);

    /**
     * Preprocesses raw frame buffer with letterbox padding into planar float output vector.
     * Output vector size is guaranteed to be (3 * target_width * target_height).
     * Returns LetterboxInfo containing transformation parameters to un-map bounding boxes.
     */
    LetterboxInfo preprocess(const VideoFrame& frame, std::vector<float>& out_planar_rgb) const;

    [[nodiscard]] int target_width() const noexcept { return target_width_; }
    [[nodiscard]] int target_height() const noexcept { return target_height_; }

private:
    int target_width_{640};
    int target_height_{640};
};

} // namespace degoonification::linux_backend
