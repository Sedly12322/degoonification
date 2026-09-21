#pragma once

#include "engine_shared/bounding_box.hpp"
#include "../capture/video_frame.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <mutex>

namespace degoonification::linux_backend {

struct AnchorPixel {
    int x{0};
    int y{0};
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
};

struct TrackedShield {
    core::BoundingBox box;
    uint64_t last_seen_ms{0};
    uint64_t created_ms{0};
    std::vector<AnchorPixel> anchors;
};

struct AnchorTrackerConfig {
    /// Maximum time in ms a shield persists after lost anchor match or no new detections
    uint64_t persistence_window_ms{1200};
    /// Dynamic boundary expansion ratio (+35% padding by default)
    float padding_ratio{0.35f};
    /// Minimum IoU to associate new detections with existing shields
    float merge_iou_threshold{0.15f};
    /// Maximum mean RGB error to consider the screen around the shield static
    float anchor_error_threshold{22.0f};
};

/**
 * Visual Anchor & Anti-Flicker Shield Tracker.
 *
 * Solves the screen-capture compositor feedback loop:
 * When an opaque shield is drawn on screen, the composited screen capture contains the shield,
 * blinding the AI detector from seeing the explicit content behind it.
 *
 * This tracker samples visual anchor pixels in the perimeter collar immediately OUTSIDE
 * the shield. As long as those border pixels match (screen is static, user hasn't navigated away),
 * the shield is kept continuously active with ZERO flicker and ZERO buffer swaps.
 * When the user scrolls vertically, the tracker detects the scroll offset and glides
 * the shield along with the content.
 */
class VisualAnchorTracker {
public:
    explicit VisualAnchorTracker(AnchorTrackerConfig config = {})
        : config_(config) {}

    void set_config(AnchorTrackerConfig config) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }

    [[nodiscard]] AnchorTrackerConfig config() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

    /**
     * Ingests the current captured frame and raw detections from YOLO at current_time_ms.
     * Evaluates anchors, incorporates new detections, prunes stale shields,
     * and returns the final expanded, merged bounding boxes for overlay rendering.
     */
    std::vector<core::BoundingBox> process_frame(
        const VideoFrame& frame,
        const std::vector<core::BoundingBox>& raw_detections,
        uint64_t current_time_ms
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 1. Evaluate visual anchors for existing active shields
        if (frame.data && frame.width > 0 && frame.height > 0) {
            for (auto& shield : shields_) {
                if (shield.anchors.empty()) continue;

                // Check static position (dy = 0)
                float static_err = eval_anchors(frame, shield.anchors, 0);
                if (static_err < config_.anchor_error_threshold) {
                    // Content surrounding the shield is unchanged -> lock shield active!
                    shield.last_seen_ms = current_time_ms;
                    continue;
                }

                // Check vertical scrolling offsets: dy in {-80, -60, -40, -20, -10, 10, 20, 40, 60, 80}
                int best_dy = 0;
                float best_err = static_err;
                const int test_offsets[] = {-80, -60, -40, -20, -10, 10, 20, 40, 60, 80};
                for (int dy : test_offsets) {
                    float err = eval_anchors(frame, shield.anchors, dy);
                    if (err < best_err) {
                        best_err = err;
                        best_dy = dy;
                    }
                }

                if (best_err < config_.anchor_error_threshold && best_dy != 0) {
                    // Content scrolled vertically by best_dy -> glide shield with scroll!
                    float norm_dy = static_cast<float>(best_dy) / frame.height;
                    shield.box.y = std::clamp(shield.box.y + norm_dy, 0.0f, 1.0f - shield.box.height);
                    for (auto& a : shield.anchors) {
                        a.y += best_dy;
                    }
                    shield.last_seen_ms = current_time_ms;
                }
            }
        }

        // 2. Ingest newly detected explicit bounding boxes from YOLO
        for (const auto& det : raw_detections) {
            bool matched = false;
            for (auto& shield : shields_) {
                if (shield.box.overlaps(det, config_.merge_iou_threshold)) {
                    shield.box = shield.box.united(det);
                    shield.last_seen_ms = current_time_ms;
                    if (frame.data && frame.width > 0) {
                        shield.anchors = sample_anchors(frame, shield.box);
                    }
                    matched = true;
                    break;
                }
            }

            if (!matched) {
                TrackedShield new_shield;
                new_shield.box = det;
                new_shield.created_ms = current_time_ms;
                new_shield.last_seen_ms = current_time_ms;
                if (frame.data && frame.width > 0) {
                    new_shield.anchors = sample_anchors(frame, det);
                }
                shields_.push_back(std::move(new_shield));
            }
        }

        // 3. Purge shields whose anchors no longer match and have expired
        shields_.erase(
            std::remove_if(shields_.begin(), shields_.end(), [&](const TrackedShield& s) {
                return (current_time_ms < s.last_seen_ms) ||
                       ((current_time_ms - s.last_seen_ms) >= config_.persistence_window_ms);
            }),
            shields_.end()
        );

        // 4. Build expanded and merged bounding boxes for overlay
        std::vector<core::BoundingBox> expanded;
        expanded.reserve(shields_.size());
        for (const auto& s : shields_) {
            expanded.push_back(s.box.expanded(config_.padding_ratio));
        }

        return merge_overlapping_boxes(std::move(expanded), config_.merge_iou_threshold);
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        shields_.clear();
    }

    [[nodiscard]] size_t active_count() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return shields_.size();
    }

private:
    AnchorTrackerConfig config_;
    mutable std::mutex mutex_;
    std::vector<TrackedShield> shields_;

    std::vector<AnchorPixel> sample_anchors(
        const VideoFrame& frame,
        const core::BoundingBox& box
    ) const {
        if (!frame.data || frame.width == 0 || frame.height == 0) return {};

        core::BoundingBox exp = box.expanded(config_.padding_ratio);

        int sx1 = std::clamp(static_cast<int>(exp.x * frame.width), 0, static_cast<int>(frame.width) - 1);
        int sy1 = std::clamp(static_cast<int>(exp.y * frame.height), 0, static_cast<int>(frame.height) - 1);
        int sx2 = std::clamp(static_cast<int>((exp.x + exp.width) * frame.width), 0, static_cast<int>(frame.width) - 1);
        int sy2 = std::clamp(static_cast<int>((exp.y + exp.height) * frame.height), 0, static_cast<int>(frame.height) - 1);

        const int margin = 14; // pixels outside shield boundary
        std::vector<AnchorPixel> pts;
        pts.reserve(24);

        auto add_pt = [&](int px, int py) {
            if (px >= 0 && px < static_cast<int>(frame.width) &&
                py >= 0 && py < static_cast<int>(frame.height)) {
                uint8_t r, g, b;
                get_pixel_rgb(frame, px, py, r, g, b);
                pts.push_back(AnchorPixel{px, py, r, g, b});
            }
        };

        const int n_samples = 6;
        for (int i = 1; i <= n_samples; ++i) {
            int px = sx1 + i * (sx2 - sx1) / (n_samples + 1);
            add_pt(px, sy1 - margin);
            add_pt(px, sy2 + margin);
        }
        for (int i = 1; i <= n_samples; ++i) {
            int py = sy1 + i * (sy2 - sy1) / (n_samples + 1);
            add_pt(sx1 - margin, py);
            add_pt(sx2 + margin, py);
        }

        return pts;
    }

    float eval_anchors(
        const VideoFrame& frame,
        const std::vector<AnchorPixel>& anchors,
        int dy_offset
    ) const {
        if (!frame.data || anchors.empty()) return 999.0f;

        float sum_diff = 0.0f;
        int valid_count = 0;

        for (const auto& pt : anchors) {
            int ny = pt.y + dy_offset;
            if (ny >= 0 && ny < static_cast<int>(frame.height) &&
                pt.x >= 0 && pt.x < static_cast<int>(frame.width)) {
                uint8_t cur_r, cur_g, cur_b;
                get_pixel_rgb(frame, pt.x, ny, cur_r, cur_g, cur_b);
                sum_diff += (std::abs(cur_r - pt.r) + std::abs(cur_g - pt.g) + std::abs(cur_b - pt.b)) / 3.0f;
                valid_count++;
            }
        }

        return valid_count >= 4 ? (sum_diff / valid_count) : 999.0f;
    }

    static inline void get_pixel_rgb(
        const VideoFrame& frame,
        int px, int py,
        uint8_t& r, uint8_t& g, uint8_t& b
    ) noexcept {
        const int bpp = (frame.format == PixelFormat::RGB ? 3 : 4);
        const uint8_t* p = frame.data + (py * frame.stride) + (px * bpp);
        if (frame.format == PixelFormat::RGB) {
            r = p[0]; g = p[1]; b = p[2];
        } else {
            b = p[0]; g = p[1]; r = p[2];
        }
    }

    static std::vector<core::BoundingBox> merge_overlapping_boxes(
        std::vector<core::BoundingBox> boxes,
        float threshold
    ) {
        if (boxes.size() <= 1) return boxes;

        bool merged_any = true;
        while (merged_any) {
            merged_any = false;
            std::vector<core::BoundingBox> next_stage;
            std::vector<bool> consumed(boxes.size(), false);

            for (size_t i = 0; i < boxes.size(); ++i) {
                if (consumed[i]) continue;

                core::BoundingBox current = boxes[i];
                for (size_t j = i + 1; j < boxes.size(); ++j) {
                    if (consumed[j]) continue;

                    if (current.overlaps(boxes[j], threshold)) {
                        current = current.united(boxes[j]);
                        consumed[j] = true;
                        merged_any = true;
                    }
                }
                next_stage.push_back(current);
            }
            boxes = std::move(next_stage);
        }
        return boxes;
    }
};

} // namespace degoonification::linux_backend
