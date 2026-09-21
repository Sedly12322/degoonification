#include "engine_shared/radix_tree.hpp"
#include <fstream>
#include <algorithm>
#include <cctype>

namespace degoonification::core {

DomainRadixTree::DomainRadixTree()
    : root_(std::make_unique<Node>()) {}

DomainRadixTree::~DomainRadixTree() = default;

DomainRadixTree::DomainRadixTree(DomainRadixTree&&) noexcept = default;
DomainRadixTree& DomainRadixTree::operator=(DomainRadixTree&&) noexcept = default;

std::vector<std::string> DomainRadixTree::split_domain_labels(std::string_view domain) {
    std::vector<std::string> labels;
    size_t start = 0;

    // Trim trailing dots or whitespaces
    while (!domain.empty() && (domain.back() == '.' || std::isspace(static_cast<unsigned char>(domain.back())))) {
        domain.remove_suffix(1);
    }
    while (!domain.empty() && std::isspace(static_cast<unsigned char>(domain.front()))) {
        domain.remove_prefix(1);
    }

    for (size_t i = 0; i <= domain.size(); ++i) {
        if (i == domain.size() || domain[i] == '.') {
            if (i > start) {
                std::string label;
                label.reserve(i - start);
                for (size_t j = start; j < i; ++j) {
                    label.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(domain[j]))));
                }
                labels.push_back(std::move(label));
            }
            start = i + 1;
        }
    }
    return labels;
}

void DomainRadixTree::insert(std::string_view domain) {
    auto labels = split_domain_labels(domain);
    if (labels.empty()) return;

    Node* current = root_.get();
    // Traverse in reverse order: TLD -> SLD -> Subdomain
    for (auto it = labels.rbegin(); it != labels.rend(); ++it) {
        if (current->is_terminal) {
            // A broader parent domain is already blocked (e.g. badsite.com blocks sub.badsite.com)
            return;
        }

        auto& child = current->children[*it];
        if (!child) {
            child = std::make_unique<Node>();
        }
        current = child.get();
    }

    if (!current->is_terminal) {
        current->is_terminal = true;
        // Optimization: Once this node is terminal, any previously added subdomains under it are redundant
        current->children.clear();
        ++total_rules_;
    }
}

bool DomainRadixTree::matches(std::string_view domain) const noexcept {
    auto labels = split_domain_labels(domain);
    if (labels.empty() || !root_) return false;

    const Node* current = root_.get();
    for (auto it = labels.rbegin(); it != labels.rend(); ++it) {
        auto child_it = current->children.find(*it);
        if (child_it == current->children.end()) {
            return false;
        }
        current = child_it->second.get();
        if (current->is_terminal) {
            return true; // Match found (exact domain or blocked parent domain)
        }
    }

    return current->is_terminal;
}

size_t DomainRadixTree::load_from_file(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in) return 0;

    size_t count = 0;
    std::string line;
    while (std::getline(in, line)) {
        // Strip comments
        auto hash_pos = line.find('#');
        if (hash_pos != std::string::npos) {
            line.erase(hash_pos);
        }

        // Clean up common hosts file formats: "0.0.0.0 badsite.com" or "127.0.0.1 badsite.com"
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;

        std::string_view sv(&line[start], line.size() - start);
        if (sv.starts_with("0.0.0.0") || sv.starts_with("127.0.0.1")) {
            auto space_pos = sv.find_first_of(" \t");
            if (space_pos != std::string_view::npos) {
                sv = sv.substr(space_pos);
                start = sv.find_first_not_of(" \t");
                if (start != std::string_view::npos) {
                    sv = sv.substr(start);
                }
            }
        }

        if (!sv.empty()) {
            insert(sv);
            ++count;
        }
    }
    return count;
}

void DomainRadixTree::clear() {
    root_ = std::make_unique<Node>();
    total_rules_ = 0;
}

} // namespace degoonification::core
