#include "engine_shared/bloom_filter.hpp"
#include <cmath>
#include <fstream>
#include <algorithm>

namespace degoonification::core {

BloomFilter::BloomFilter(size_t expected_elements, double false_positive_rate) {
    if (expected_elements == 0) expected_elements = 1;
    if (false_positive_rate <= 0.0 || false_positive_rate >= 1.0) {
        false_positive_rate = 0.001;
    }

    // Formula: m = - (n * ln(p)) / (ln(2)^2)
    const double ln2 = 0.6931471805599453;
    const double ln2_sq = ln2 * ln2;
    double m = -static_cast<double>(expected_elements) * std::log(false_positive_rate) / ln2_sq;
    num_bits_ = static_cast<size_t>(std::ceil(m));
    if (num_bits_ < 64) num_bits_ = 64;

    // Formula: k = (m / n) * ln(2)
    double k = (static_cast<double>(num_bits_) / static_cast<double>(expected_elements)) * ln2;
    num_hashes_ = static_cast<size_t>(std::max(1.0, std::round(k)));

    // Allocate uint64_t words
    size_t words = (num_bits_ + 63) / 64;
    bits_.assign(words, 0);
    num_bits_ = words * 64; // align to 64 bits
    element_count_ = 0;
}

uint64_t BloomFilter::hash_fnv1a_64(std::string_view s) noexcept {
    uint64_t hash = 14695981039346656037ULL;
    for (char c : s) {
        // Normalize lowercase
        auto uc = static_cast<uint8_t>(std::tolower(static_cast<unsigned char>(c)));
        hash ^= uc;
        hash *= 1099511628211ULL;
    }
    return hash;
}

uint64_t BloomFilter::hash_murmur_mix_64(std::string_view s) noexcept {
    uint64_t h = 0x517cc1b727220a95ULL;
    for (char c : s) {
        auto uc = static_cast<uint8_t>(std::tolower(static_cast<unsigned char>(c)));
        h ^= uc;
        h *= 0xc6a4a7935bd1e995ULL;
        h ^= h >> 47;
    }
    return h;
}

void BloomFilter::add(std::string_view key) {
    if (num_bits_ == 0) return;

    uint64_t h1 = hash_fnv1a_64(key);
    uint64_t h2 = hash_murmur_mix_64(key);

    for (size_t i = 0; i < num_hashes_; ++i) {
        uint64_t combined = (h1 + i * h2) % num_bits_;
        bits_[combined / 64] |= (1ULL << (combined % 64));
    }
    ++element_count_;
}

bool BloomFilter::contains(std::string_view key) const noexcept {
    if (num_bits_ == 0 || bits_.empty()) return false;

    uint64_t h1 = hash_fnv1a_64(key);
    uint64_t h2 = hash_murmur_mix_64(key);

    for (size_t i = 0; i < num_hashes_; ++i) {
        uint64_t combined = (h1 + i * h2) % num_bits_;
        if (!(bits_[combined / 64] & (1ULL << (combined % 64)))) {
            return false;
        }
    }
    return true;
}

void BloomFilter::clear() {
    std::fill(bits_.begin(), bits_.end(), 0);
    element_count_ = 0;
}

bool BloomFilter::save_to_file(const std::string& filepath) const {
    std::ofstream out(filepath, std::ios::binary);
    if (!out) return false;

    // Header: Magic "DGBF", version 1
    const char magic[4] = {'D', 'G', 'B', 'F'};
    const uint32_t version = 1;
    out.write(magic, 4);
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));

    uint64_t n_bits = num_bits_;
    uint64_t n_hashes = num_hashes_;
    uint64_t n_elem = element_count_;
    uint64_t n_words = bits_.size();

    out.write(reinterpret_cast<const char*>(&n_bits), sizeof(n_bits));
    out.write(reinterpret_cast<const char*>(&n_hashes), sizeof(n_hashes));
    out.write(reinterpret_cast<const char*>(&n_elem), sizeof(n_elem));
    out.write(reinterpret_cast<const char*>(&n_words), sizeof(n_words));

    if (!bits_.empty()) {
        out.write(reinterpret_cast<const char*>(bits_.data()), bits_.size() * sizeof(uint64_t));
    }
    return out.good();
}

bool BloomFilter::load_from_file(const std::string& filepath) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in) return false;

    char magic[4];
    uint32_t version = 0;
    in.read(magic, 4);
    in.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic[0] != 'D' || magic[1] != 'G' || magic[2] != 'B' || magic[3] != 'F' || version != 1) {
        return false;
    }

    uint64_t n_bits = 0, n_hashes = 0, n_elem = 0, n_words = 0;
    in.read(reinterpret_cast<char*>(&n_bits), sizeof(n_bits));
    in.read(reinterpret_cast<char*>(&n_hashes), sizeof(n_hashes));
    in.read(reinterpret_cast<char*>(&n_elem), sizeof(n_elem));
    in.read(reinterpret_cast<char*>(&n_words), sizeof(n_words));

    std::vector<uint64_t> loaded_bits(n_words);
    if (n_words > 0) {
        in.read(reinterpret_cast<char*>(loaded_bits.data()), n_words * sizeof(uint64_t));
    }

    if (!in) return false;

    num_bits_ = n_bits;
    num_hashes_ = n_hashes;
    element_count_ = n_elem;
    bits_ = std::move(loaded_bits);
    return true;
}

} // namespace degoonification::core
