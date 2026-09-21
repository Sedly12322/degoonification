#include "../dns_packet.hpp"
#include <cassert>
#include <iostream>
#include <arpa/inet.h>

using namespace degoonification::network;

void test_packet_parse_and_synthesize() {
    // Construct a standard raw DNS query for "pornhub.com", type A (1), class IN (1), ID 0x1234
    std::vector<uint8_t> raw_query = {
        0x12, 0x34,             // ID = 0x1234
        0x01, 0x00,             // Flags: Standard query, RD=1
        0x00, 0x01,             // QDCOUNT = 1
        0x00, 0x00,             // ANCOUNT = 0
        0x00, 0x00,             // NSCOUNT = 0
        0x00, 0x00,             // ARCOUNT = 0
        // QNAME: 7pornhub3com0
        0x07, 'p', 'o', 'r', 'n', 'h', 'u', 'b',
        0x03, 'c', 'o', 'm',
        0x00,
        0x00, 0x01,             // QTYPE = 1 (A)
        0x00, 0x01              // QCLASS = 1 (IN)
    };

    DnsPacket packet;
    bool ok = DnsPacket::parse(raw_query.data(), raw_query.size(), packet);
    assert(ok);
    assert(packet.id == 0x1234);
    assert(packet.questions.size() == 1);
    assert(packet.questions[0].qname == "pornhub.com");
    assert(packet.questions[0].qtype == 1);

    // Build sinkhole response
    auto resp = packet.build_sinkhole_response();
    assert(resp.size() > 12);

    // Verify response ID matches
    uint16_t resp_id = (static_cast<uint16_t>(resp[0]) << 8) | resp[1];
    assert(resp_id == 0x1234);

    // Verify QR=1, RCODE=0
    assert((resp[2] & 0x80) != 0); // Response bit set
    assert((resp[3] & 0x0F) == 0); // NoError

    // Verify Answer RDATA is 0.0.0.0 (last 4 bytes)
    size_t rdata_offset = resp.size() - 4;
    assert(resp[rdata_offset + 0] == 0);
    assert(resp[rdata_offset + 1] == 0);
    assert(resp[rdata_offset + 2] == 0);
    assert(resp[rdata_offset + 3] == 0);

    std::cout << "[PASS] test_packet_parse_and_synthesize (Returns 0.0.0.0)\n";
}

int main() {
    test_packet_parse_and_synthesize();
    std::cout << "All DNS network tests passed successfully!\n";
    return 0;
}
