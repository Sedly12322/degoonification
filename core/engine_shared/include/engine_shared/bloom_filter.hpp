#pragma once

#include <vector>
#include <string_view>
#include <string>
#include <cstdint>
#include <cstddef>

namespace degoonification::core {

/**
 * High-performance Bloom Filter for sub-millisecond adult domain lookups.
 * Uses Kirsch-Mitzenmacher double-hashing technique for optimal CPU cache utilization.
 */
class BloomFilter {
public:
    /**
     * Initializes bloom filter sized for given expected element count and desired false positive probability.
     */
    explicit BloomFilter(size_t expected_elements = 100000, double false_positive_rate = 0.001);
    ~BloomFilter() = default;

    BloomFilter(const BloomFilter&) = default;
    BloomFilter& operator=(const BloomFilter&) = default;
    BloomFilter(BloomFilter&&) noexcept = default;
    BloomFilter& operator=(BloomFilter&&) noexcept = default;

    /**
     * Adds an entry (e.g. domain string) to the filter.
     */
    void add(std::string_view key);

    /**
     * Checks if an entry is possibly in the set.
     * Returns false if definitely not present; true if probably present.
     */
    [[nodiscard]] bool contains(std::string_view key) const noexcept;

    /**
     * Clears all bits.
     */
    void clear();

    [[nodiscard]] size_t bit_count() const noexcept { return num_bits_; }
    [[nodiscard]] size_t hash_count() const noexcept { return num_hashes_; }
    [[nodiscard]] size_t element_count() const noexcept { return element_count_; }

    /**
     * Binary serialization for fast zero-overhead cold startup.
     */
    bool save_to_file(const std::string& filepath) const;
    bool load_from_file(const std::string& filepath);

private:
    size_t num_bits_{0};
    size_t num_hashes_{0};
    size_t element_count_{0};
    std::vector<uint64_t> bits_;

    static uint64_t hash_fnv1a_64(std::string_view s) noexcept;
    static uint64_t hash_murmur_mix_64(std::string_view s) noexcept;
};

} // namespace degoonification::core
