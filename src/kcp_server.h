#ifndef _KCP_SERVER_H_INCLUDED
#define _KCP_SERVER_H_INCLUDED

#include <map>

#include "kcp_interface.h"
#include "kcp_stream.h"
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

    // udp callback
    void OnRecvUdp(const UDPAddress& from, const char *buf, size_t len) override;
    bool OnError(int err, const char *what) override;

    std::shared_ptr<UDPInterface> udp_;
    KCPServerCallback *cb_ = nullptr;

    class UDPAdapter;
    std::map<UDPAddress, UDPAdapter *> clients_;

    bool stopped_ = true;
};
}

#endif // !_KCP_SERVER_H_INCLUDED
