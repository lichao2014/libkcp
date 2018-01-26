#ifndef _KCP_CLIENT_H_INCLUDED
#define _KCP_CLIENT_H_INCLUDED

#include <vector>

#include "kcp_interface.h"
#include "udp_interface.h"
#include "kcp_stream.h"
#include "kcp_error.h"

namespace kcp {
class KCPClient : public KCPClientInterface {
public:
    static std::shared_ptr<KCPClient> Create(std::shared_ptr<UDPInterface> udp);

    bool Connect(const UDPAddress& to,
                 uint32_t conv,
                 const KCPConfig& config,
                 KCPClientCallback *cb) override;
    bool Write(const char *buf, std::size_t len) override;
    void Close() override;
    const UDPAddress& local_address() const override;
    const UDPAddress& remote_address() const override;
    uint32_t conv() const override;
    ExecutorInterface *executor() override;
private:
    explicit KCPClient(std::shared_ptr<UDPInterface> udp)
        : udp_(udp) {}

    ~KCPClient() = default;

    std::shared_ptr<KCPStream> stream_;
    std::shared_ptr<UDPInterface> udp_;
};
}

#endif // !_KCP_CLIENT_H_INCLUDED
