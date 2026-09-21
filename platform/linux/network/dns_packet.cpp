#include "dns_packet.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <cctype>

namespace degoonification::network {

bool DnsPacket::parse(const uint8_t* data, size_t len, DnsPacket& out_packet) {
    if (len < 12) return false;

    out_packet.id = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    out_packet.flags = (static_cast<uint16_t>(data[2]) << 8) | data[3];
    uint16_t qdcount = (static_cast<uint16_t>(data[4]) << 8) | data[5];

    out_packet.questions.clear();
    size_t offset = 12;

    for (uint16_t q = 0; q < qdcount; ++q) {
        std::string qname;
        size_t hops = 0;
        size_t curr = offset;
        bool jumped = false;

        while (curr < len && hops < 64) {
            uint8_t label_len = data[curr];
            if (label_len == 0) {
                if (!jumped) offset = curr + 1;
                break;
            }

            if ((label_len & 0xC0) == 0xC0) {
                // Compression pointer
                if (curr + 1 >= len) return false;
                uint16_t ptr_offset = ((label_len & 0x3F) << 8) | data[curr + 1];
                if (!jumped) offset = curr + 2;
                jumped = true;
                curr = ptr_offset;
                ++hops;
                continue;
            }

            ++curr;
            if (curr + label_len > len) return false;

            if (!qname.empty()) qname.push_back('.');
            for (size_t i = 0; i < label_len; ++i) {
                qname.push_back(static_cast<char>(std::tolower(data[curr + i])));
            }
            curr += label_len;
            if (!jumped) offset = curr;
            ++hops;
        }

        if (offset + 4 > len) return false;
        uint16_t qtype = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
        uint16_t qclass = (static_cast<uint16_t>(data[offset + 2]) << 8) | data[offset + 3];
        offset += 4;

        out_packet.questions.push_back(DnsQuestion{
            .qname = std::move(qname),
            .qtype = qtype,
            .qclass = qclass
        });
    }

    return true;
}

std::vector<uint8_t> DnsPacket::build_sinkhole_response() const {
    if (questions.empty()) return {};

    std::vector<uint8_t> response;
    response.reserve(128);

    // 1. Header (12 bytes)
    response.push_back(static_cast<uint8_t>(id >> 8));
    response.push_back(static_cast<uint8_t>(id & 0xFF));

    // Flags: 0x8180 = QR(1) | RD(1) | RA(1) | RCODE(0)
    response.push_back(0x81);
    response.push_back(0x80);

    // QDCOUNT = 1
    response.push_back(0x00);
    response.push_back(0x01);

    // ANCOUNT = 1
    response.push_back(0x00);
    response.push_back(0x01);

    // NSCOUNT = 0, ARCOUNT = 0
    response.push_back(0x00);
    response.push_back(0x00);
    response.push_back(0x00);
    response.push_back(0x00);

    const auto& q = questions[0];

    // 2. Question section (reconstruct QNAME)
    size_t start = 0;
    while (start < q.qname.size()) {
        size_t dot = q.qname.find('.', start);
        if (dot == std::string::npos) dot = q.qname.size();
        uint8_t label_len = static_cast<uint8_t>(dot - start);
        response.push_back(label_len);
        for (size_t i = start; i < dot; ++i) {
            response.push_back(static_cast<uint8_t>(q.qname[i]));
        }
        start = dot + 1;
    }
    response.push_back(0x00); // end of qname

    // QTYPE & QCLASS
    response.push_back(static_cast<uint8_t>(q.qtype >> 8));
    response.push_back(static_cast<uint8_t>(q.qtype & 0xFF));
    response.push_back(static_cast<uint8_t>(q.qclass >> 8));
    response.push_back(static_cast<uint8_t>(q.qclass & 0xFF));

    // 3. Answer section
    // Pointer to QNAME: 0xC00C (offset 12)
    response.push_back(0xC0);
    response.push_back(0x0C);

    if (q.qtype == 28) { // AAAA (IPv6)
        response.push_back(0x00);
        response.push_back(0x1C); // Type AAAA = 28
        response.push_back(0x00);
        response.push_back(0x01); // Class IN = 1
        // TTL = 300s
        response.push_back(0x00);
        response.push_back(0x00);
        response.push_back(0x01);
        response.push_back(0x2C);
        // RDLENGTH = 16
        response.push_back(0x00);
        response.push_back(0x10);
        // IP: :: (16 zeroes)
        for (int i = 0; i < 16; ++i) response.push_back(0x00);
    } else { // A (IPv4) or default
        response.push_back(0x00);
        response.push_back(0x01); // Type A = 1
        response.push_back(0x00);
        response.push_back(0x01); // Class IN = 1
        // TTL = 300s
        response.push_back(0x00);
        response.push_back(0x00);
        response.push_back(0x01);
        response.push_back(0x2C);
        // RDLENGTH = 4
        response.push_back(0x00);
        response.push_back(0x04);
        // IP: 0.0.0.0
        response.push_back(0x00);
        response.push_back(0x00);
        response.push_back(0x00);
        response.push_back(0x00);
    }

    return response;
}

} // namespace degoonification::network
