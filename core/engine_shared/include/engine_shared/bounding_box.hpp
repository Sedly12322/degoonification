#pragma once

#include <cstdint>
#include <algorithm>

namespace degoonification::core {

/**
 * Normalized Bounding Box [0.0, 1.0] representing coordinates on screen.
 * Coordinates are:
 *  - x: left position (0.0 to 1.0)
 *  - y: top position (0.0 to 1.0)
 *  - width: horizontal extent (0.0 to 1.0)
 *  - height: vertical extent (0.0 to 1.0)
 */
struct BoundingBox {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
    float confidence{0.0f};
    int32_t class_id{0};
    uint64_t timestamp_ms{0};

    [[nodiscard]] constexpr float left() const noexcept { return x; }
    [[nodiscard]] constexpr float top() const noexcept { return y; }
    [[nodiscard]] constexpr float right() const noexcept { return x + width; }
    [[nodiscard]] constexpr float bottom() const noexcept { return y + height; }
    [[nodiscard]] constexpr float area() const noexcept { return width * height; }

    /**
     * Expands the bounding box outward by a given ratio (e.g. +15% = 0.15f)
     * keeping the center anchored, and clamps boundaries strictly to [0.0, 1.0].
     */
    [[nodiscard]] BoundingBox expanded(float ratio = 0.15f) const noexcept {
        float dw = width * ratio;
        float dh = height * ratio;

        float nx1 = std::max(0.0f, x - (dw * 0.5f));
        float ny1 = std::max(0.0f, y - (dh * 0.5f));
        float nx2 = std::min(1.0f, x + width + (dw * 0.5f));
        float ny2 = std::min(1.0f, y + height + (dh * 0.5f));

        return BoundingBox{
            .x = nx1,
            .y = ny1,
            .width = std::max(0.0f, nx2 - nx1),
            .height = std::max(0.0f, ny2 - ny1),
            .confidence = confidence,
            .class_id = class_id,
            .timestamp_ms = timestamp_ms
        };
    }

    /**
     * Calculates Intersection over Union (IoU) with another bounding box.
     */
    [[nodiscard]] float iou(const BoundingBox& other) const noexcept {
        float ix1 = std::max(left(), other.left());
        float iy1 = std::max(top(), other.top());
        float ix2 = std::min(right(), other.right());
        float iy2 = std::min(bottom(), other.bottom());

        float iw = std::max(0.0f, ix2 - ix1);
        float ih = std::max(0.0f, iy2 - iy1);
        float intersection = iw * ih;

        float union_area = area() + other.area() - intersection;
        if (union_area <= 0.0f) {
            return 0.0f;
        }
        return intersection / union_area;
    }

    /**
     * Checks whether this box overlaps significantly with another.
     */
    [[nodiscard]] bool overlaps(const BoundingBox& other, float threshold = 0.2f) const noexcept {
        return iou(other) > threshold;
    }

    /**
     * Returns union bounding box containing both.
     */
    [[nodiscard]] BoundingBox united(const BoundingBox& other) const noexcept {
        float nx1 = std::min(left(), other.left());
        float ny1 = std::min(top(), other.top());
        float nx2 = std::max(right(), other.right());
        float ny2 = std::max(bottom(), other.bottom());

        return BoundingBox{
            .x = nx1,
            .y = ny1,
            .width = std::max(0.0f, nx2 - nx1),
            .height = std::max(0.0f, ny2 - ny1),
            .confidence = std::max(confidence, other.confidence),
            .class_id = confidence >= other.confidence ? class_id : other.class_id,
            .timestamp_ms = std::max(timestamp_ms, other.timestamp_ms)
        };
    }
};

} // namespace degoonification::core
