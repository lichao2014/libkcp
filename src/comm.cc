#include "kcp_interface.h"

#include <cassert>
#include <chrono>

namespace kcp {
uint64_t Now64() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        high_resolution_clock::now().time_since_epoch()).count();
}

uint32_t Now32() {
    return static_cast<uint32_t>(Now64());
}
}

using namespace kcp;

namespace {
std::string IP4ToString(uint32_t ip4) {
    char buf[std::numeric_limits<uint8_t>::digits10 * 4 + 3 + 1];

    uint8_t *p = reinterpret_cast<uint8_t *>(&ip4);
    snprintf(buf, sizeof buf, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);

    return buf;
}

bool IP4FromString(const char *str, uint32_t *ip4) {
    int a[4];
    if (4 != sscanf_s(str, "%d.%d.%d.%d", a, a + 1, a + 2, a + 3)) {
        return false;
    }

    uint8_t *p = reinterpret_cast<uint8_t *>(ip4);
    p[0] = a[0];
    p[1] = a[1];
    p[2] = a[2];
    p[3] = a[3];

    return true;
}
}

UDPAddress::UDPAddress(const char *ip4, uint16_t port, uint16_t conv)
    : ip4(0)
    , port(port)
    , conv(conv) {
    assert(IP4FromString(ip4, &this->ip4));
}

std::string UDPAddress::ip4_string() const {
    return IP4ToString(ip4);
}