#pragma once

#include "../capture/video_frame.hpp"
#include <vector>
#include <cstdint>

namespace degoonification::linux_backend {

/**
 * High-performance frame preprocessor.
 * Converts captured screen pixels (BGRA/RGBA) into planar float32 RGB [3 x 640 x 640]
 * normalized to [0.0, 1.0] for the YOLOv8 ONNX inference engine.
 */
class ImagePreprocessor {
public:
    ImagePreprocessor(int target_width = 640, int target_height = 640);

    /**
     * Preprocesses raw frame buffer directly into planar float output vector.
     * Output vector size is guaranteed to be (3 * target_width * target_height).
     */
    void preprocess(const VideoFrame& frame, std::vector<float>& out_planar_rgb) const;

    [[nodiscard]] int target_width() const noexcept { return target_width_; }
    [[nodiscard]] int target_height() const noexcept { return target_height_; }

private:
    int target_width_{640};
    int target_height_{640};
};

} // namespace degoonification::linux_backend
