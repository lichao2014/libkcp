#ifndef _KCP_PROXY_H_INCLUDED
#define _KCP_PROXY_H_INCLUDED

#include <unordered_map>

#include "udp_interface.h"

namespace kcp {
using StreamKey = IP4Address;

struct StreamKeyHasher {
    constexpr size_t operator() (const StreamKey& key) const noexcept {
        return key.u64;
    }
};

class KCPMux : public UDPCallback
             , public UDPInterface
             , public std::enable_shared_from_this<KCPMux> {
public:
    static std::shared_ptr<KCPMux> Create(std::shared_ptr<UDPInterface> udp, bool bind_callback);

    std::shared_ptr<UDPInterface> AddUDPFilter(const StreamKey& key);

    // udp interface
    bool Open(UDPCallback *cb) override;
    void Close() override;
    void SetRecvBufSize(size_t recv_size) override;
    bool Send(const IP4Address& to, const char *buf, size_t len) override;
    const IP4Address& local_address() const override;
    ExecutorInterface *executor() override;

    // udp callback
    bool OnRecvUDP(const IP4Address& from, const char *buf, size_t len) override;
    bool OnError(const std::error_code& ec) override;
private:
    explicit KCPMux(std::shared_ptr<UDPInterface> udp) : udp_(udp) {}
    ~KCPMux();

    bool BindUDPCallback();

    std::shared_ptr<UDPInterface> udp_;

    class UDPAdapter;
    std::unordered_map<StreamKey, UDPAdapter *, StreamKeyHasher> by_key_streams_;

    bool udp_callback_binded_ = false;
};

using IP4AddressHasher = StreamKeyHasher;

class KCPProxy {
public:
    std::shared_ptr<KCPMux> AddMux(IOContextInterface *io_ctx, const IP4Address& addr);
    std::shared_ptr<UDPInterface> AddUDPFilter(IOContextInterface *io_ctx,
                                               const IP4Address& addr,
                                               const IP4Address& peer,
                                               uint32_t conv);
    bool IsMuxAddress(const IP4Address& addr) const;
private:
    std::unordered_map<IP4Address, std::weak_ptr<KCPMux>, IP4AddressHasher> by_address_muxes_;
};
}

#endif // !_KCP_PROXY_H_INCLUDED
