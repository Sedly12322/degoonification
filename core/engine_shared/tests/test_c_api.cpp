#include "engine_shared/c_api.h"
#include <cassert>
#include <iostream>

void test_c_api() {
    // 1. Tracker C API
    dg_tracker_t* tracker = dg_tracker_create(300, 0.15f, 0.25f);
    assert(tracker != nullptr);

    dg_bounding_box_t input_box{
        .x = 0.1f,
        .y = 0.1f,
        .width = 0.2f,
        .height = 0.2f,
        .confidence = 0.95f,
        .class_id = 1,
        .timestamp_ms = 50
    };

    dg_bounding_box_t out_boxes[16];
    int32_t count = dg_tracker_process(tracker, &input_box, 1, 50, out_boxes, 16);
    assert(count == 1);
    assert(out_boxes[0].width > 0.2f); // expanded

    dg_tracker_destroy(tracker);

    // 2. Bloom Filter C API
    dg_bloom_t* bf = dg_bloom_create(1000, 0.001);
    assert(bf != nullptr);
    dg_bloom_add(bf, "nsfw.test");
    assert(dg_bloom_contains(bf, "nsfw.test"));
    assert(!dg_bloom_contains(bf, "safe.test"));
    dg_bloom_destroy(bf);

    // 3. Radix Tree C API
    dg_radix_t* radix = dg_radix_create();
    assert(radix != nullptr);
    dg_radix_insert(radix, "badservice.io");
    assert(dg_radix_matches(radix, "badservice.io"));
    assert(dg_radix_matches(radix, "stream.badservice.io"));
    assert(!dg_radix_matches(radix, "goodservice.io"));
    dg_radix_destroy(radix);

    std::cout << "[PASS] test_c_api\n";
}

int main() {
    test_c_api();
    std::cout << "All C API tests passed!\n";
    return 0;
}
