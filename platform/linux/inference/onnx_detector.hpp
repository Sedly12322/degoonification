#pragma once

#include "engine_shared/bounding_box.hpp"
#include "image_preprocessor.hpp"
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace degoonification::linux_backend {

struct DetectorConfig {
    std::string model_path{"core/models/yolov8n-nsfw.onnx"};
    float confidence_threshold{0.25f};
    float nms_iou_threshold{0.45f};
    int input_width{640};
    int input_height{640};
    int num_threads{4};
    bool use_gpu{true};
};

/**
 * High-performance ONNX Runtime inference engine for YOLOv8n-NSFW.
 */
class OnnxDetector {
public:
    explicit OnnxDetector(DetectorConfig config = {});
    ~OnnxDetector();

    OnnxDetector(const OnnxDetector&) = delete;
    OnnxDetector& operator=(const OnnxDetector&) = delete;
    OnnxDetector(OnnxDetector&&) noexcept;
    OnnxDetector& operator=(OnnxDetector&&) noexcept;

    /**
     * Initializes the ONNX session with hardware acceleration if available.
     * Returns true on success.
     */
    bool init();

    /**
     * Runs inference on planar RGB float32 buffer [3 x 640 x 640] normalized to [0.0, 1.0].
     * Un-maps letterboxed coordinates back to original frame [0.0, 1.0].
     * Returns detected explicit bounding boxes in normalized coordinates [0.0, 1.0].
     */
    std::vector<core::BoundingBox> detect(
        const float* planar_rgb_data,
        const LetterboxInfo& letterbox = LetterboxInfo{}
    );

    /**
     * Checks if class_id represents explicit/NSFW content requiring blur.
     */
    [[nodiscard]] static bool is_explicit_class(int32_t class_id) noexcept;

    [[nodiscard]] bool is_initialized() const noexcept { return is_initialized_; }
    [[nodiscard]] const DetectorConfig& config() const noexcept { return config_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    DetectorConfig config_;
    bool is_initialized_{false};
};

} // namespace degoonification::linux_backend
