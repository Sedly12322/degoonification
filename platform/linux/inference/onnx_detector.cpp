#include "onnx_detector.hpp"
#include <onnxruntime_cxx_api.h>
#include <algorithm>
#include <iostream>
#include <cmath>

namespace degoonification::linux_backend {

struct OnnxDetector::Impl {
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "DegoonificationDetector"};
    Ort::SessionOptions session_options;
    std::unique_ptr<Ort::Session> session;
    Ort::MemoryInfo memory_info{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)};

    std::vector<std::string> input_node_names;
    std::vector<std::string> output_node_names;
    std::vector<const char*> input_names_ptrs;
    std::vector<const char*> output_names_ptrs;

    int64_t input_shape[4]{1, 3, 640, 640};
};

OnnxDetector::OnnxDetector(DetectorConfig config)
    : config_(std::move(config)) {}

OnnxDetector::~OnnxDetector() = default;

OnnxDetector::OnnxDetector(OnnxDetector&&) noexcept = default;
OnnxDetector& OnnxDetector::operator=(OnnxDetector&&) noexcept = default;

bool OnnxDetector::init() {
    try {
        impl_ = std::make_unique<Impl>();
        impl_->session_options.SetIntraOpNumThreads(config_.num_threads);
        impl_->session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        impl_->input_shape[2] = config_.input_height;
        impl_->input_shape[3] = config_.input_width;

        // Create Session
        impl_->session = std::make_unique<Ort::Session>(
            impl_->env,
            config_.model_path.c_str(),
            impl_->session_options
        );

        // Fetch input/output metadata
        Ort::AllocatorWithDefaultOptions allocator;
        size_t num_inputs = impl_->session->GetInputCount();
        for (size_t i = 0; i < num_inputs; ++i) {
            auto name = impl_->session->GetInputNameAllocated(i, allocator);
            impl_->input_node_names.emplace_back(name.get());
        }

        size_t num_outputs = impl_->session->GetOutputCount();
        for (size_t i = 0; i < num_outputs; ++i) {
            auto name = impl_->session->GetOutputNameAllocated(i, allocator);
            impl_->output_node_names.emplace_back(name.get());
        }

        for (const auto& name : impl_->input_node_names) {
            impl_->input_names_ptrs.push_back(name.c_str());
        }
        for (const auto& name : impl_->output_node_names) {
            impl_->output_names_ptrs.push_back(name.c_str());
        }

        is_initialized_ = true;
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[OnnxDetector] Init error: " << ex.what() << "\n";
        is_initialized_ = false;
        return false;
    }
}

bool OnnxDetector::is_explicit_class(int32_t class_id) noexcept {
    // Classes from YOLOv8n-NSFW model:
    // 0: FEMALE_GENITALIA_COVERED
    // 2: BUTTOCKS_EXPOSED
    // 3: FEMALE_BREAST_EXPOSED
    // 4: FEMALE_GENITALIA_EXPOSED
    // 6: ANUS_EXPOSED
    // 13: BELLY_EXPOSED
    // 14: MALE_GENITALIA_EXPOSED
    // 15: ANUS_COVERED
    // 16: FEMALE_BREAST_COVERED
    // 17: BUTTOCKS_COVERED
    switch (class_id) {
        case 0:
        case 2:
        case 3:
        case 4:
        case 6:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            return true;
        default:
            return false;
    }
}

std::vector<core::BoundingBox> OnnxDetector::detect(
    const float* planar_rgb_data,
    const LetterboxInfo& letterbox
) {
    if (!is_initialized_ || !impl_ || !impl_->session || !planar_rgb_data) {
        return {};
    }

    try {
        size_t input_tensor_size = 3 * config_.input_width * config_.input_height;
        auto input_tensor = Ort::Value::CreateTensor<float>(
            impl_->memory_info,
            const_cast<float*>(planar_rgb_data),
            input_tensor_size,
            impl_->input_shape,
            4
        );

        auto output_tensors = impl_->session->Run(
            Ort::RunOptions{nullptr},
            impl_->input_names_ptrs.data(),
            &input_tensor,
            1,
            impl_->output_names_ptrs.data(),
            impl_->output_names_ptrs.size()
        );

        if (output_tensors.empty() || !output_tensors[0].IsTensor()) {
            return {};
        }

        float* out_data = output_tensors[0].GetTensorMutableData<float>();
        auto type_info = output_tensors[0].GetTensorTypeAndShapeInfo();
        auto shape = type_info.GetShape();

        // YOLOv8 shape format is usually [1, 4 + classes, 8400]
        if (shape.size() < 3) return {};

        int64_t rows = shape[1]; // channels / classes + 4
        int64_t cols = shape[2]; // number of proposals (8400)
        int64_t num_classes = rows - 4;

        std::vector<core::BoundingBox> candidates;

        float scaled_w = letterbox.scaled_w > 0 ? static_cast<float>(letterbox.scaled_w) : static_cast<float>(config_.input_width);
        float scaled_h = letterbox.scaled_h > 0 ? static_cast<float>(letterbox.scaled_h) : static_cast<float>(config_.input_height);

        for (int64_t col = 0; col < cols; ++col) {
            float max_score = 0.0f;
            int32_t best_class = -1;

            for (int64_t c = 0; c < num_classes; ++c) {
                float score = out_data[(4 + c) * cols + col];
                if (score > max_score) {
                    max_score = score;
                    best_class = static_cast<int32_t>(c);
                }
            }

            if (max_score >= config_.confidence_threshold && is_explicit_class(best_class)) {
                float cx = out_data[0 * cols + col];
                float cy = out_data[1 * cols + col];
                float w  = out_data[2 * cols + col];
                float h  = out_data[3 * cols + col];

                // Un-map letterbox coords back to original frame [0, 1]
                float norm_cx = (cx - letterbox.pad_x) / scaled_w;
                float norm_cy = (cy - letterbox.pad_y) / scaled_h;
                float norm_w  = w / scaled_w;
                float norm_h  = h / scaled_h;

                float x = std::clamp(norm_cx - (norm_w * 0.5f), 0.0f, 1.0f);
                float y = std::clamp(norm_cy - (norm_h * 0.5f), 0.0f, 1.0f);
                float box_w = std::clamp(norm_w, 0.0f, 1.0f - x);
                float box_h = std::clamp(norm_h, 0.0f, 1.0f - y);

                if (box_w > 0.005f && box_h > 0.005f) {
                    candidates.push_back(core::BoundingBox{
                        .x = x,
                        .y = y,
                        .width = box_w,
                        .height = box_h,
                        .confidence = max_score,
                        .class_id = best_class,
                        .timestamp_ms = 0
                    });
                }
            }
        }

        // Apply NMS (Non-Maximum Suppression)
        std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            return a.confidence > b.confidence;
        });

        std::vector<core::BoundingBox> nms_results;
        std::vector<bool> suppressed(candidates.size(), false);

        for (size_t i = 0; i < candidates.size(); ++i) {
            if (suppressed[i]) continue;
            nms_results.push_back(candidates[i]);

            for (size_t j = i + 1; j < candidates.size(); ++j) {
                if (suppressed[j]) continue;
                if (candidates[i].iou(candidates[j]) > config_.nms_iou_threshold) {
                    suppressed[j] = true;
                }
            }
        }

        return nms_results;
    } catch (const std::exception& ex) {
        std::cerr << "[OnnxDetector] Detect error: " << ex.what() << "\n";
        return {};
    }
}

} // namespace degoonification::linux_backend
