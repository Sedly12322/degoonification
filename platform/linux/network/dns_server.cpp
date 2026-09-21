#include "dns_server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <iostream>
#include <cstring>

namespace degoonification::network {

DnsServer::DnsServer(DnsServerConfig config)
    : config_(std::move(config)), bloom_filter_(50000, 0.001) {}

DnsServer::~DnsServer() {
    stop();
}

DnsStats DnsServer::stats() const noexcept {
    return DnsStats{
        .total_queries = total_queries_.load(),
        .blocked_queries = blocked_queries_.load(),
        .forwarded_queries = forwarded_queries_.load()
    };
}

bool DnsServer::init() {
    // 1. Load domain blocklist into Radix Tree & Bloom Filter
    size_t loaded = radix_tree_.load_from_file(config_.blocklist_path);
    std::cout << "[DnsServer] Loaded " << loaded << " adult domain rules into Radix Tree.\n";

    // 2. Create and bind listening UDP socket
    server_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "[DnsServer] Failed to create socket: " << std::strerror(errno) << "\n";
        return false;
    }

    int reuse = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config_.listen_port);
    inet_pton(AF_INET, config_.listen_ip.c_str(), &addr.sin_addr);

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[DnsServer] Failed to bind to " << config_.listen_ip << ":" << config_.listen_port
                  << " (" << std::strerror(errno) << ")\n";
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    // Set non-blocking
    fcntl(server_fd_, F_SETFL, fcntl(server_fd_, F_GETFL, 0) | O_NONBLOCK);

    // 3. Create upstream socket
    upstream_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (upstream_fd_ >= 0) {
        fcntl(upstream_fd_, F_SETFL, fcntl(upstream_fd_, F_GETFL, 0) | O_NONBLOCK);
    }

    return true;
}

bool DnsServer::start() {
    if (server_fd_ < 0 || is_running_.load()) return false;

    is_running_.store(true);
    worker_thread_ = std::make_unique<std::thread>(&DnsServer::run_loop, this);
    return true;
}

void DnsServer::stop() {
    if (!is_running_.exchange(false)) return;

    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
        worker_thread_.reset();
    }

    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
    if (upstream_fd_ >= 0) {
        close(upstream_fd_);
        upstream_fd_ = -1;
    }
}

void DnsServer::run_loop() {
    uint8_t buffer[2048];
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    sockaddr_in upstream_addr{};
    upstream_addr.sin_family = AF_INET;
    upstream_addr.sin_port = htons(config_.upstream_dns_port);
    inet_pton(AF_INET, config_.upstream_dns_ip.c_str(), &upstream_addr.sin_addr);

    struct pollfd pfd{};
    pfd.fd = server_fd_;
    pfd.events = POLLIN;

    while (is_running_.load()) {
        int ret = poll(&pfd, 1, 100); // 100ms timeout
        if (ret <= 0) continue;

        if (pfd.revents & POLLIN) {
            ssize_t bytes = recvfrom(server_fd_, buffer, sizeof(buffer), 0,
                                     reinterpret_cast<sockaddr*>(&client_addr), &client_len);
            if (bytes <= 0) continue;

            total_queries_++;

            DnsPacket packet;
            if (!DnsPacket::parse(buffer, bytes, packet) || packet.questions.empty()) {
                continue;
            }

            const auto& qname = packet.questions[0].qname;
            bool is_blocked = radix_tree_.matches(qname) || bloom_filter_.contains(qname);

            if (is_blocked) {
                blocked_queries_++;
                auto sink_resp = packet.build_sinkhole_response();
                sendto(server_fd_, sink_resp.data(), sink_resp.size(), 0,
                       reinterpret_cast<sockaddr*>(&client_addr), client_len);
            } else {
                forwarded_queries_++;
                // Forward query to upstream DNS
                if (upstream_fd_ >= 0) {
                    sendto(upstream_fd_, buffer, bytes, 0,
                           reinterpret_cast<sockaddr*>(&upstream_addr), sizeof(upstream_addr));

                    // Wait briefly for upstream response
                    struct pollfd up_pfd{};
                    up_pfd.fd = upstream_fd_;
                    up_pfd.events = POLLIN;

                    if (poll(&up_pfd, 1, 300) > 0 && (up_pfd.revents & POLLIN)) {
                        uint8_t up_buf[2048];
                        ssize_t up_bytes = recvfrom(upstream_fd_, up_buf, sizeof(up_buf), 0, nullptr, nullptr);
                        if (up_bytes > 0) {
                            sendto(server_fd_, up_buf, up_bytes, 0,
                                   reinterpret_cast<sockaddr*>(&client_addr), client_len);
                        }
                    }
                }
            }
        }
    }
}

} // namespace degoonification::network
