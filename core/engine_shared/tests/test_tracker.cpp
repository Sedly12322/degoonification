#include "engine_shared/temporal_smoothing.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace degoonification::core;

void test_bounding_box_expansion() {
    BoundingBox box{
        .x = 0.5f,
        .y = 0.5f,
        .width = 0.2f,
        .height = 0.2f,
        .confidence = 0.9f,
        .class_id = 1,
        .timestamp_ms = 100
    };

    auto expanded = box.expanded(0.15f); // +15% expansion
    // dw = 0.2 * 0.15 = 0.03
    // expected x = 0.5 - 0.015 = 0.485
    // expected width = 0.23
    assert(std::abs(expanded.width - 0.23f) < 1e-4);
    assert(std::abs(expanded.height - 0.23f) < 1e-4);
    assert(std::abs(expanded.x - 0.485f) < 1e-4);
    assert(std::abs(expanded.y - 0.485f) < 1e-4);

    // Test clamping at edges
    BoundingBox edge_box{
        .x = 0.05f,
        .y = 0.95f,
        .width = 0.2f,
        .height = 0.2f
    };
    auto clamped = edge_box.expanded(0.20f);
    assert(clamped.x >= 0.0f);
    assert(clamped.bottom() <= 1.0f);

    std::cout << "[PASS] test_bounding_box_expansion\n";
}

void test_iou() {
    BoundingBox a{.x = 0.0f, .y = 0.0f, .width = 0.5f, .height = 0.5f};
    BoundingBox b{.x = 0.25f, .y = 0.0f, .width = 0.5f, .height = 0.5f};

    float iou = a.iou(b);
    // Intersection = 0.25 * 0.5 = 0.125
    // Union = 0.25 + 0.25 - 0.125 = 0.375
    // IoU = 0.125 / 0.375 = 1/3 ~ 0.3333
    assert(std::abs(iou - (1.0f / 3.0f)) < 1e-4);

    BoundingBox c{.x = 0.8f, .y = 0.8f, .width = 0.1f, .height = 0.1f};
    assert(a.iou(c) == 0.0f);

    std::cout << "[PASS] test_iou\n";
}

void test_temporal_persistence() {
    TemporalSmoothingTracker tracker(TrackerConfig{
        .persistence_window_ms = 300,
        .padding_ratio = 0.15f,
        .merge_iou_threshold = 0.2f
    });

    BoundingBox detection{
        .x = 0.2f,
        .y = 0.2f,
        .width = 0.3f,
        .height = 0.3f,
        .confidence = 0.85f,
        .class_id = 0
    };

    // Frame at t = 100 ms: detection present
    auto res1 = tracker.process_frame({detection}, 100);
    assert(res1.size() == 1);
    assert(tracker.active_count() == 1);

    // Frame at t = 250 ms: no new detections (e.g. dropped frame or scroll)
    // Box must persist because delta = 150 ms < 300 ms
    auto res2 = tracker.process_frame({}, 250);
    assert(res2.size() == 1);
    assert(tracker.active_count() == 1);

    // Frame at t = 405 ms: no new detections
    // Box must expire because delta = 305 ms >= 300 ms
    auto res3 = tracker.process_frame({}, 405);
    assert(res3.empty());
    assert(tracker.active_count() == 0);

    std::cout << "[PASS] test_temporal_persistence\n";
}

int main() {
    test_bounding_box_expansion();
    test_iou();
    test_temporal_persistence();
    std::cout << "All Tracker tests passed!\n";
    return 0;
}
