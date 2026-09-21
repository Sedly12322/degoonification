#ifndef DEGOONIFICATION_C_API_H
#define DEGOONIFICATION_C_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef DEGOONIFICATION_BUILD_DLL
        #define DG_EXPORT __declspec(dllexport)
    #else
        #define DG_EXPORT
    #endif
#else
    #define DG_EXPORT __attribute__((visibility("default")))
#endif

/**
 * C-compatible Bounding Box representation.
 */
typedef struct {
    float x;
    float y;
    float width;
    float height;
    float confidence;
    int32_t class_id;
    uint64_t timestamp_ms;
} dg_bounding_box_t;

// Opaque handles
typedef struct dg_tracker_t dg_tracker_t;
typedef struct dg_bloom_t dg_bloom_t;
typedef struct dg_radix_t dg_radix_t;

/* --- Temporal Smoothing & Anti-Flicker Tracker --- */

DG_EXPORT dg_tracker_t* dg_tracker_create(
    uint64_t persistence_window_ms,
    float padding_ratio,
    float merge_iou_threshold
);

DG_EXPORT void dg_tracker_destroy(dg_tracker_t* tracker);

/**
 * Ingests detections and populates out_boxes array up to max_out_count.
 * Returns the actual count of rendered boxes written to out_boxes.
 */
DG_EXPORT int32_t dg_tracker_process(
    dg_tracker_t* tracker,
    const dg_bounding_box_t* in_boxes,
    int32_t in_count,
    uint64_t timestamp_ms,
    dg_bounding_box_t* out_boxes,
    int32_t max_out_count
);

DG_EXPORT void dg_tracker_reset(dg_tracker_t* tracker);
DG_EXPORT uint64_t dg_tracker_active_count(const dg_tracker_t* tracker);

/* --- Bloom Filter --- */

DG_EXPORT dg_bloom_t* dg_bloom_create(uint64_t expected_elements, double fp_rate);
DG_EXPORT void dg_bloom_destroy(dg_bloom_t* bf);
DG_EXPORT void dg_bloom_add(dg_bloom_t* bf, const char* key);
DG_EXPORT bool dg_bloom_contains(const dg_bloom_t* bf, const char* key);
DG_EXPORT bool dg_bloom_save(const dg_bloom_t* bf, const char* filepath);
DG_EXPORT bool dg_bloom_load(dg_bloom_t* bf, const char* filepath);

/* --- Domain Radix Tree --- */

DG_EXPORT dg_radix_t* dg_radix_create(void);
DG_EXPORT void dg_radix_destroy(dg_radix_t* tree);
DG_EXPORT void dg_radix_insert(dg_radix_t* tree, const char* domain);
DG_EXPORT bool dg_radix_matches(const dg_radix_t* tree, const char* domain);
DG_EXPORT uint64_t dg_radix_load_file(dg_radix_t* tree, const char* filepath);
DG_EXPORT uint64_t dg_radix_size(const dg_radix_t* tree);

#ifdef __cplusplus
}
#endif

#endif // DEGOONIFICATION_C_API_H
