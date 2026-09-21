#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <vector>
#include <cstdint>

namespace degoonification::core {

/**
 * Domain Suffix/Radix Tree for hierarchical domain matching.
 * By indexing domain labels in reverse order (e.g., "com" -> "example" -> "sub"),
 * any query for "a.b.example.com" immediately matches "example.com" in O(depth) steps.
 */
class DomainRadixTree {
public:
    DomainRadixTree();
    ~DomainRadixTree();

    DomainRadixTree(const DomainRadixTree&) = delete;
    DomainRadixTree& operator=(const DomainRadixTree&) = delete;
    DomainRadixTree(DomainRadixTree&&) noexcept;
    DomainRadixTree& operator=(DomainRadixTree&&) noexcept;

    /**
     * Inserts a domain rule (e.g., "badsite.com").
     * Automatically covers "badsite.com" and all subdomains like "media.badsite.com".
     */
    void insert(std::string_view domain);

    /**
     * Checks if a domain or any of its parent domains is blocked.
     * e.g., if "badsite.com" is blocked, both "badsite.com" and "cdn.sub.badsite.com" return true.
     */
    [[nodiscard]] bool matches(std::string_view domain) const noexcept;

    /**
     * Loads domains line-by-line from a text blocklist file.
     * Ignores comments (#) and empty lines. Returns number of added domains.
     */
    size_t load_from_file(const std::string& filepath);

    [[nodiscard]] size_t size() const noexcept { return total_rules_; }
    void clear();

private:
    struct Node {
        bool is_terminal{false};
        std::unordered_map<std::string, std::unique_ptr<Node>> children;
    };

    std::unique_ptr<Node> root_;
    size_t total_rules_{0};

    static std::vector<std::string> split_domain_labels(std::string_view domain);
};

} // namespace degoonification::core
