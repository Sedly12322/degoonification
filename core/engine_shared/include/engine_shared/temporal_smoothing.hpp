#pragma once

#include "bounding_box.hpp"
#include <vector>
#include <mutex>
#include <cstdint>

namespace degoonification::core {

struct TrackerConfig {
    /// Maximum time in milliseconds a box remains active after detection (hysteresis window)
    uint64_t persistence_window_ms{800};
    /// Dynamic outward boundary expansion ratio (e.g., 0.35 = +35%)
    float padding_ratio{0.35f};
    /// Minimum IoU threshold to associate and merge matching/overlapping boxes
    float merge_iou_threshold{0.15f};
};

/**
 * Temporal Smoothing & Anti-Flicker Hysteresis Tracker.
 * Implements Box_render(t) = Box(t) U {Box(t-k) | (t - t_k) < 300ms}
 * with +15% dynamic boundary expansion to prevent flickering during rapid scrolling and animations.
 */
class TemporalSmoothingTracker {
public:
    explicit TemporalSmoothingTracker(TrackerConfig config = {});
    ~TemporalSmoothingTracker() = default;

    TemporalSmoothingTracker(const TemporalSmoothingTracker&) = delete;
    TemporalSmoothingTracker& operator=(const TemporalSmoothingTracker&) = delete;
    TemporalSmoothingTracker(TemporalSmoothingTracker&&) noexcept;
    TemporalSmoothingTracker& operator=(TemporalSmoothingTracker&&) noexcept;

    /**
     * Ingests newly detected raw bounding boxes from the AI inference engine at time `current_time_ms`,
     * purges stale boxes older than 300ms, correlates overlapping detections, applies +15% expansion,
     * merges overlapping boxes, and returns the final set of rendered blur rectangles.
     */
    std::vector<BoundingBox> process_frame(
        const std::vector<BoundingBox>& raw_detections,
        uint64_t current_time_ms
    );

    /**
     * Explicitly clears all tracked boxes.
     */
    void reset();

    /**
     * Returns count of currently active tracked boxes.
     */
    [[nodiscard]] size_t active_count() const;

    [[nodiscard]] const TrackerConfig& config() const noexcept { return config_; }
    void set_config(const TrackerConfig& config) noexcept { config_ = config; }

private:
    TrackerConfig config_;
    mutable std::mutex mutex_;
    std::vector<BoundingBox> tracked_boxes_;

    static std::vector<BoundingBox> merge_overlapping_boxes(
        std::vector<BoundingBox> boxes,
        float threshold
    );
};

} // namespace degoonification::core
