#ifndef _KCP_SERVER_H_INCLUDED
#define _KCP_SERVER_H_INCLUDED

#include "kcp_interface.h"
#include "kcp_proxy.h"
#include "udp_interface.h"

namespace kcp {
class KCPServer : public UDPCallback
                , public KCPServerInterface
                , public std::enable_shared_from_this<KCPServer> {
public:
    static std::shared_ptr<KCPServer> Create(std::shared_ptr<UDPInterface> udp);

    bool Start(KCPServerCallback *cb) override;
    void Stop() override;
    const UDPAddress& local_address() const override;
    ExecutorInterface *executor() override { return udp_->executor(); }
private:
    explicit KCPServer(std::shared_ptr<UDPInterface> udp) : udp_(udp) {};
    ~KCPServer();

    void OnNewClient(const UDPAddress& from, uint32_t conv, const char *buf, size_t len);

    // udp callback
    void OnRecvUDP(const UDPAddress& from, const char *buf, size_t len) override;
    bool OnError(const std::error_code& ec) override;

    std::shared_ptr<UDPInterface> udp_;
    std::shared_ptr<KCPProxy> proxy_;

    KCPServerCallback *cb_ = nullptr;

    bool stopped_ = true;
};
}

#endif // !_KCP_SERVER_H_INCLUDED
