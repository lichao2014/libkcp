#ifndef _KCP_SERVER_H_INCLUDED
#define _KCP_SERVER_H_INCLUDED

#include "kcp_interface.h"
#include "kcp_proxy.h"
#include "udp_interface.h"

namespace kcp {
class KCPServer : public UDPCallback {
public:
    explicit KCPServer(std::shared_ptr<UDPInterface> udp) : udp_(udp) {};
    ~KCPServer() { Stop(); }

    bool Start(KCPServerCallback *cb);
    void Stop();
    const UDPAddress& local_address() const;
    ExecutorInterface *executor()    { return udp_->executor(); }
private:
    void OnNewClient(const UDPAddress& from, uint32_t conv, const char *buf, size_t len);

    // udp callback
    void OnRecvUDP(const UDPAddress& from, const char *buf, size_t len) override;
    bool OnError(const std::error_code& ec) override;

    std::shared_ptr<UDPInterface> udp_;
    std::shared_ptr<KCPProxy> proxy_;

    KCPServerCallback *cb_ = nullptr;
    bool stopped_ = true;
};

class KCPServerAdapter : public KCPServerInterface {
public:
    static std::shared_ptr<KCPServerAdapter> Create(std::shared_ptr<UDPInterface> udp);

    bool Start(KCPServerCallback *cb) override {
        return executor()->Invoke(&KCPServer::Start, impl_.get(), cb);
    }

    void Stop() override {
        executor()->Invoke(&KCPServer::Stop, impl_.get());
    }

    const UDPAddress& local_address() const override {
        return impl_->local_address();
    }

    ExecutorInterface *executor() override {
        return impl_->executor();
    }
private:
    explicit KCPServerAdapter(std::unique_ptr<KCPServer> impl)
        : impl_(std::move(impl)) {}

    ~KCPServerAdapter() {
        executor()->Invoke([this] {
            impl_->Stop();
            impl_.reset();
        });
    }

    std::unique_ptr<KCPServer> impl_;
};
}

#endif // !_KCP_SERVER_H_INCLUDED
