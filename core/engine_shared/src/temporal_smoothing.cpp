#include "engine_shared/temporal_smoothing.hpp"
#include <algorithm>

namespace degoonification::core {

TemporalSmoothingTracker::TemporalSmoothingTracker(TrackerConfig config)
    : config_(config) {}

TemporalSmoothingTracker::TemporalSmoothingTracker(TemporalSmoothingTracker&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.mutex_);
    config_ = other.config_;
    tracked_boxes_ = std::move(other.tracked_boxes_);
}

TemporalSmoothingTracker& TemporalSmoothingTracker::operator=(TemporalSmoothingTracker&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(mutex_, other.mutex_);
        config_ = other.config_;
        tracked_boxes_ = std::move(other.tracked_boxes_);
    }
    return *this;
}

std::vector<BoundingBox> TemporalSmoothingTracker::process_frame(
    const std::vector<BoundingBox>& raw_detections,
    uint64_t current_time_ms
) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 1. Purge stale boxes where (current_time_ms - timestamp_ms) >= persistence_window_ms
    tracked_boxes_.erase(
        std::remove_if(
            tracked_boxes_.begin(),
            tracked_boxes_.end(),
            [this, current_time_ms](const BoundingBox& b) {
                return (current_time_ms < b.timestamp_ms) ||
                       ((current_time_ms - b.timestamp_ms) >= config_.persistence_window_ms);
            }
        ),
        tracked_boxes_.end()
    );

    // 2. Ingest raw detections with updated timestamp
    for (auto det : raw_detections) {
        det.timestamp_ms = current_time_ms;

        // Check if it correlates with an existing tracked box
        bool matched = false;
        for (auto& existing : tracked_boxes_) {
            if (existing.overlaps(det, config_.merge_iou_threshold)) {
                // Smooth/unite existing with new detection
                existing = existing.united(det);
                existing.timestamp_ms = current_time_ms;
                matched = true;
                break;
            }
        }

        if (!matched) {
            tracked_boxes_.push_back(det);
        }
    }

    // 3. Apply +15% dynamic boundary expansion to all active boxes
    std::vector<BoundingBox> expanded_boxes;
    expanded_boxes.reserve(tracked_boxes_.size());
    for (const auto& box : tracked_boxes_) {
        expanded_boxes.push_back(box.expanded(config_.padding_ratio));
    }

    // 4. Merge overlapping expanded boxes to minimize draw calls / blur shader passes
    return merge_overlapping_boxes(std::move(expanded_boxes), config_.merge_iou_threshold);
}

std::vector<BoundingBox> TemporalSmoothingTracker::merge_overlapping_boxes(
    std::vector<BoundingBox> boxes,
    float threshold
) {
    if (boxes.size() <= 1) {
        return boxes;
    }

    bool merged_any = true;
    while (merged_any) {
        merged_any = false;
        std::vector<BoundingBox> next_stage;
        std::vector<bool> consumed(boxes.size(), false);

        for (size_t i = 0; i < boxes.size(); ++i) {
            if (consumed[i]) continue;

            BoundingBox current = boxes[i];
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

void TemporalSmoothingTracker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    tracked_boxes_.clear();
}

size_t TemporalSmoothingTracker::active_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tracked_boxes_.size();
}

} // namespace degoonification::core
