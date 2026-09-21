#include "engine_shared/bloom_filter.hpp"
#include "engine_shared/radix_tree.hpp"
#include <cassert>
#include <iostream>
#include <cstdio>

using namespace degoonification::core;

void test_bloom_filter() {
    BloomFilter bf(1000, 0.001);

    bf.add("pornsite.com");
    bf.add("xxx-content.net");
    bf.add("explicit-domain.org");

    assert(bf.contains("pornsite.com"));
    assert(bf.contains("xxx-content.net"));
    assert(bf.contains("explicit-domain.org"));

    // Case insensitivity
    assert(bf.contains("PORNSITE.COM"));
    assert(bf.contains("Xxx-Content.Net"));

    // Not in filter
    assert(!bf.contains("google.com"));
    assert(!bf.contains("wikipedia.org"));
    assert(!bf.contains("github.com"));

    // Serialization test
    const std::string tmp_file = "/tmp/test_bloom.bin";
    assert(bf.save_to_file(tmp_file));

    BloomFilter loaded_bf(1, 0.1);
    assert(loaded_bf.load_from_file(tmp_file));
    assert(loaded_bf.contains("pornsite.com"));
    assert(!loaded_bf.contains("google.com"));
    std::remove(tmp_file.c_str());

    std::cout << "[PASS] test_bloom_filter\n";
}

void test_radix_tree() {
    DomainRadixTree tree;

    tree.insert("pornsite.com");
    tree.insert("nsfw-hub.net");

    // Exact matches
    assert(tree.matches("pornsite.com"));
    assert(tree.matches("nsfw-hub.net"));

    // Subdomain matches (must block all subdomains)
    assert(tree.matches("www.pornsite.com"));
    assert(tree.matches("cdn.video.pornsite.com"));
    assert(tree.matches("login.nsfw-hub.net"));

    // Case insensitivity
    assert(tree.matches("WWW.PORNSITE.COM"));
    assert(tree.matches("Cdn.Video.PornSite.Com"));

    // Must NOT match similar or parent domains
    assert(!tree.matches("notpornsite.com"));
    assert(!tree.matches("pornsite.com.org"));
    assert(!tree.matches("com"));
    assert(!tree.matches("google.com"));

    std::cout << "[PASS] test_radix_tree\n";
}

int main() {
    test_bloom_filter();
    test_radix_tree();
    std::cout << "All DNS tests passed!\n";
    return 0;
}
