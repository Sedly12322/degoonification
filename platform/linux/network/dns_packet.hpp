#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace degoonification::network {

struct DnsQuestion {
    std::string qname;
    uint16_t qtype{1};   // 1 = A, 28 = AAAA
    uint16_t qclass{1};  // 1 = IN
};

struct DnsPacket {
    uint16_t id{0};
    uint16_t flags{0};
    std::vector<DnsQuestion> questions;

    /**
     * Parses raw UDP DNS packet. Returns true if successfully parsed.
     */
    static bool parse(const uint8_t* data, size_t len, DnsPacket& out_packet);

    /**
     * Constructs an instantaneous sinkhole response returning 0.0.0.0 (A) or :: (AAAA).
     */
    [[nodiscard]] std::vector<uint8_t> build_sinkhole_response() const;
};

} // namespace degoonification::network
