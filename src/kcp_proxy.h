#ifndef _KCP_PROXY_H_INCLUDED
#define _KCP_PROXY_H_INCLUDED

#include <map>

#include "udp_interface.h"

namespace kcp {
using StreamKey = UDPAddress;

class KCPProxy : public std::enable_shared_from_this<KCPProxy> {
public:
    enum Result : uint8_t {
        kSuccess,
        kNotFound,
        kBadMsg
    };

    static std::shared_ptr<KCPProxy> Create(std::shared_ptr<UDPInterface> udp);

    std::shared_ptr<UDPInterface> AddFilter(const StreamKey& key);
    Result RecvUDP(const UDPAddress& from, const char *buf, size_t len, uint32_t *conv);
private:
    explicit KCPProxy(std::shared_ptr<UDPInterface> udp) : udp_(udp) {}
    ~KCPProxy() = default;

    static bool ParseConv(const char *buf, size_t len, uint32_t *conv);

    std::shared_ptr<UDPInterface> udp_;

    class UDPAdapter;
    std::map<StreamKey, UDPAdapter *> by_key_streams_;
};
}

#endif // !_KCP_PROXY_H_INCLUDED
