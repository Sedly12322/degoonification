#include "engine_shared/c_api.h"
#include "engine_shared/temporal_smoothing.hpp"
#include "engine_shared/bloom_filter.hpp"
#include "engine_shared/radix_tree.hpp"

using namespace degoonification::core;

struct dg_tracker_t {
    TemporalSmoothingTracker tracker;
};

struct dg_bloom_t {
    BloomFilter bf;
};

struct dg_radix_t {
    DomainRadixTree tree;
};

extern "C" {

dg_tracker_t* dg_tracker_create(
    uint64_t persistence_window_ms,
    float padding_ratio,
    float merge_iou_threshold
) {
    TrackerConfig config{
        .persistence_window_ms = persistence_window_ms == 0 ? 300 : persistence_window_ms,
        .padding_ratio = padding_ratio <= 0.0f ? 0.15f : padding_ratio,
        .merge_iou_threshold = merge_iou_threshold <= 0.0f ? 0.25f : merge_iou_threshold,
    };
    return new dg_tracker_t{TemporalSmoothingTracker(config)};
}

void dg_tracker_destroy(dg_tracker_t* tracker) {
    delete tracker;
}

int32_t dg_tracker_process(
    dg_tracker_t* tracker,
    const dg_bounding_box_t* in_boxes,
    int32_t in_count,
    uint64_t timestamp_ms,
    dg_bounding_box_t* out_boxes,
    int32_t max_out_count
) {
    if (!tracker || !out_boxes || max_out_count <= 0) return 0;

    std::vector<BoundingBox> inputs;
    if (in_boxes && in_count > 0) {
        inputs.reserve(in_count);
        for (int32_t i = 0; i < in_count; ++i) {
            inputs.push_back(BoundingBox{
                .x = in_boxes[i].x,
                .y = in_boxes[i].y,
                .width = in_boxes[i].width,
                .height = in_boxes[i].height,
                .confidence = in_boxes[i].confidence,
                .class_id = in_boxes[i].class_id,
                .timestamp_ms = in_boxes[i].timestamp_ms,
            });
        }
    }

    auto results = tracker->tracker.process_frame(inputs, timestamp_ms);
    int32_t count = std::min(static_cast<int32_t>(results.size()), max_out_count);

    for (int32_t i = 0; i < count; ++i) {
        out_boxes[i] = dg_bounding_box_t{
            .x = results[i].x,
            .y = results[i].y,
            .width = results[i].width,
            .height = results[i].height,
            .confidence = results[i].confidence,
            .class_id = results[i].class_id,
            .timestamp_ms = results[i].timestamp_ms,
        };
    }

    return count;
}

void dg_tracker_reset(dg_tracker_t* tracker) {
    if (tracker) {
        tracker->tracker.reset();
    }
}

uint64_t dg_tracker_active_count(const dg_tracker_t* tracker) {
    if (!tracker) return 0;
    return tracker->tracker.active_count();
}

dg_bloom_t* dg_bloom_create(uint64_t expected_elements, double fp_rate) {
    return new dg_bloom_t{BloomFilter(expected_elements, fp_rate)};
}

void dg_bloom_destroy(dg_bloom_t* bf) {
    delete bf;
}

void dg_bloom_add(dg_bloom_t* bf, const char* key) {
    if (bf && key) {
        bf->bf.add(key);
    }
}

bool dg_bloom_contains(const dg_bloom_t* bf, const char* key) {
    if (!bf || !key) return false;
    return bf->bf.contains(key);
}

bool dg_bloom_save(const dg_bloom_t* bf, const char* filepath) {
    if (!bf || !filepath) return false;
    return bf->bf.save_to_file(filepath);
}

bool dg_bloom_load(dg_bloom_t* bf, const char* filepath) {
    if (!bf || !filepath) return false;
    return bf->bf.load_from_file(filepath);
}

dg_radix_t* dg_radix_create(void) {
    return new dg_radix_t{DomainRadixTree()};
}

void dg_radix_destroy(dg_radix_t* tree) {
    delete tree;
}

void dg_radix_insert(dg_radix_t* tree, const char* domain) {
    if (tree && domain) {
        tree->tree.insert(domain);
    }
}

bool dg_radix_matches(const dg_radix_t* tree, const char* domain) {
    if (!tree || !domain) return false;
    return tree->tree.matches(domain);
}

uint64_t dg_radix_load_file(dg_radix_t* tree, const char* filepath) {
    if (!tree || !filepath) return 0;
    return tree->tree.load_from_file(filepath);
}

uint64_t dg_radix_size(const dg_radix_t* tree) {
    if (!tree) return 0;
    return tree->tree.size();
}

} // extern "C"
