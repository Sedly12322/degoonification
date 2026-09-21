#pragma once

#include "dns_packet.hpp"
#include "engine_shared/radix_tree.hpp"
#include "engine_shared/bloom_filter.hpp"
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <cstdint>

namespace degoonification::network {

struct DnsServerConfig {
    std::string listen_ip{"127.0.0.1"};
    uint16_t listen_port{5353};
    std::string upstream_dns_ip{"1.1.1.1"};
    uint16_t upstream_dns_port{53};
    std::string blocklist_path{"core/blocklists/default_domains.txt"};
};

struct DnsStats {
    uint64_t total_queries{0};
    uint64_t blocked_queries{0};
    uint64_t forwarded_queries{0};
};

/**
 * High-performance local DNS Sinkhole Server.
 * Intercepts port 53 DNS queries and drops/redirects adult domains to 0.0.0.0 in < 0.2 ms
 * using the shared DomainRadixTree and BloomFilter.
 */
class DnsServer {
public:
    explicit DnsServer(DnsServerConfig config = {});
    ~DnsServer();

    DnsServer(const DnsServer&) = delete;
    DnsServer& operator=(const DnsServer&) = delete;

    /**
     * Initializes the server socket and loads blocklists.
     */
    bool init();

    /**
     * Starts the UDP worker loop in a background thread.
     */
    bool start();

    /**
     * Stops the server.
     */
    void stop();

    [[nodiscard]] bool is_running() const noexcept { return is_running_.load(); }
    [[nodiscard]] DnsStats stats() const noexcept;

private:
    DnsServerConfig config_;
    std::atomic<bool> is_running_{false};
    int server_fd_{-1};
    int upstream_fd_{-1};
    std::unique_ptr<std::thread> worker_thread_;

    core::DomainRadixTree radix_tree_;
    core::BloomFilter bloom_filter_;

    std::atomic<uint64_t> total_queries_{0};
    std::atomic<uint64_t> blocked_queries_{0};
    std::atomic<uint64_t> forwarded_queries_{0};

    void run_loop();
};

} // namespace degoonification::network
