#ifndef _KCP_CLIENT_H_INCLUDED
#define _KCP_CLIENT_H_INCLUDED

#include <vector>

#include "kcp_interface.h"
#include "udp_interface.h"
#include "kcp_stream.h"

namespace kcp {
class KCPClient : public UDPCallback
                , public TaskInterface
                , public KCPClientInterface
                , public std::enable_shared_from_this<KCPClient> {
public:
    static std::shared_ptr<KCPClient> Create(std::shared_ptr<UDPInterface> udp);

    bool Connect(const UDPAddress& to, uint32_t conv, const KCPConfig& config, KCPClientCallback *cb) override;
    bool Write(const char *buf, std::size_t len) override;
    void Close() override;
    const UDPAddress& local_address() const override;
    const UDPAddress& remote_address() const override;
    uint32_t conv() const override { return stream_->conv; }
    ExecutorInterface *executor() override { return udp_->executor(); }
private:
    explicit KCPClient(std::shared_ptr<UDPInterface> udp) : udp_(udp) {}
    ~KCPClient();

    void WriteUDP(const char *buf, std::size_t len);
    void TryRecvKCP();

    // udp callback
    void OnRecvUdp(const UDPAddress& from, const char *buf, size_t len) override;
    bool OnError(int err, const char *what) override;

    // task callback
    uint32_t OnRun(uint32_t now, bool cancel) override;

    // kcp output
    static int KCPOutput(const char *buf, int len, struct IKCPCB *kcp, void *user);

    KCPStream stream_;
    std::shared_ptr<UDPInterface> udp_;
    UDPAddress peer_;
    std::vector<char> recv_buf_;
    KCPClientCallback *cb_ = nullptr;
    bool closed_ = true;
};

}

#endif // !_KCP_CLIENT_H_INCLUDED
