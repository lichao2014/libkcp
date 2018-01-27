#ifndef _KCP_SERVER_H_INCLUDED
#define _KCP_SERVER_H_INCLUDED

#include "kcp_interface.h"
#include "kcp_proxy.h"
#include "udp_interface.h"

namespace kcp {
class KCPServer : public UDPCallback, public KCPServerInterface {
public:
    explicit KCPServer(std::shared_ptr<UDPInterface> udp, const KCPServerConfig& config)
        : udp_(udp)
        , config_(config) {}

    ~KCPServer() { Stop(); }

    bool Start(KCPServerCallback *cb) override;
    void Stop() override;
    const IP4Address& local_address() const override;
    ExecutorInterface *executor()    override;
private:
    bool OnNewClient(const IP4Address& from, uint32_t conv, const char *buf, size_t len);

    // udp callback
    bool OnRecvUDP(const IP4Address& from, const char *buf, size_t len) override;
    bool OnError(const std::error_code& ec) override;

    std::shared_ptr<UDPInterface> udp_;
    std::shared_ptr<KCPMux> proxy_;
    KCPServerConfig config_;
    KCPServerCallback *cb_ = nullptr;
    bool stopped_ = true;
};

class KCPServerAdapter : public KCPServerInterface {
public:
    static std::unique_ptr<KCPServerAdapter> Create(
        std::shared_ptr<UDPInterface> udp, const KCPServerConfig& config);

    explicit KCPServerAdapter(std::unique_ptr<KCPServer> impl)
        : impl_(std::move(impl)) {}

    ~KCPServerAdapter() {
        executor()->Invoke([this] {
            impl_->Stop();
            impl_.reset();
        });
    }

    bool Start(KCPServerCallback *cb) override {
        return executor()->Invoke(&KCPServerInterface::Start, impl_.get(), cb);
    }

    void Stop() override {
        executor()->Invoke(&KCPServerInterface::Stop, impl_.get());
    }

    const IP4Address& local_address() const override {
        return impl_->local_address();
    }

    ExecutorInterface *executor() override {
        return impl_->executor();
    }
private:
    std::unique_ptr<KCPServerInterface> impl_;
};
}

#endif // !_KCP_SERVER_H_INCLUDED
