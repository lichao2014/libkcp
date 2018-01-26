#ifndef _KCP_CONTEXT_H_INCLUDED
#define _KCP_CONTEXT_H_INCLUDED

#include "kcp_interface.h"
#include "kcp_stream.h"
#include "udp_interface.h"

namespace kcp {
class KCPContext : public KCPContextInterface {
public:
    explicit KCPContext(std::shared_ptr<IOContextInterface> io_ctx) 
        : io_ctx_(io_ctx) {}

    void Start() override;
    void Stop() override;
    ExecutorInterface *executor() override;
    std::shared_ptr<KCPClientInterface> CreateClient(const UDPAddress& addr) override;
    std::shared_ptr<KCPServerInterface> CreateServer(const UDPAddress& addr) override;
private:
    std::shared_ptr<IOContextInterface> io_ctx_;
};
}

#endif // !_KCP_CONTEXT_H_INCLUDED
