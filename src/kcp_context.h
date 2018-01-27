#ifndef _KCP_CONTEXT_H_INCLUDED
#define _KCP_CONTEXT_H_INCLUDED

#include "kcp_interface.h"
#include "kcp_proxy.h"
#include "udp_interface.h"


namespace kcp {
class KCPContext : public KCPContextInterface {
public:
    explicit KCPContext(std::shared_ptr<IOContextInterface> io_ctx)
        : io_ctx_(io_ctx) {}

    void Start() override;
    void Stop() override;
    ExecutorInterface *executor() override;
    std::unique_ptr<KCPStreamInterface> CreateStream(const IP4Address& addr, 
                                                    const IP4Address& peer, 
                                                    uint32_t conv) override;
    std::unique_ptr<KCPClientInterface> CreateClient(const IP4Address& addr, 
                                                    const IP4Address& peer, 
                                                    const KCPConfig& config) override;
    std::unique_ptr<KCPServerInterface> CreateServer(const IP4Address& addr,
                                                    const KCPServerConfig& config) override;
private:
    std::shared_ptr<IOContextInterface> io_ctx_;
    KCPProxy proxy_;
};
}

#endif // !_KCP_CONTEXT_H_INCLUDED
