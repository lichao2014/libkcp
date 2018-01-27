#ifndef _KCP_CLIENT_H_INCLUDED
#define _KCP_CLIENT_H_INCLUDED

#include "kcp_interface.h"
#include "udp_interface.h"
#include "kcp_stream.h"
#include "kcp_error.h"

namespace kcp {
class KCPClient : public KCPStreamCallback, public KCPClientInterface {
public:
    explicit KCPClient(std::shared_ptr<UDPInterface> udp,
                       const IP4Address& peer,
                       const KCPConfig& config)
        : udp_(udp)
        , peer_(peer)
        , config_(config) {}

    ~KCPClient() = default;

    bool Open(KCPClientCallback *cb) override;
    void Close() override;
    bool Write(const char *buf, std::size_t len) override;
    uint32_t conv() const override;
    const IP4Address& local_address() const override;
    const IP4Address& remote_address() const override;
    ExecutorInterface *executor() override;
private:
    // stream interface
    void OnRecvKCP(const char *buf, size_t size) override;
    bool OnError(const std::error_code& ec) override;

    std::shared_ptr<UDPInterface> udp_;
    std::unique_ptr<KCPStream> stream_;
    KCPConfig config_;
    IP4Address peer_;
    KCPClientCallback *cb_ = nullptr;
};

class KCPClientAdapter : public KCPClientInterface {
public:
    static std::unique_ptr<KCPClientAdapter> Create(std::shared_ptr<UDPInterface> udp, 
                                                    const IP4Address& peer, 
                                                    const KCPConfig& config);

    explicit KCPClientAdapter(std::unique_ptr<KCPClientInterface> impl)
        : impl_(std::move(impl)) {}

    ~KCPClientAdapter() {
        executor()->Invoke([this] {
            impl_->Close();
            impl_.reset();
        });
    }

    bool Open(KCPClientCallback *cb) override {
        return executor()->Invoke(&KCPClientInterface::Open, impl_.get(), cb);
    }

    void Close() override {
        executor()->Invoke(&KCPClientInterface::Close, impl_.get());
    }

    bool Write(const char *buf, size_t len) {
        return executor()->Invoke(&KCPClientInterface::Write, impl_.get(), buf, len);
    }

    uint32_t conv() const {
        return impl_->conv();
    }

    const IP4Address& local_address() const override {
        return impl_->local_address();
    }

    const IP4Address& remote_address() const override {
        return impl_->remote_address();
    }

    ExecutorInterface *executor() override {
        return impl_->executor();
    }
private:
    std::unique_ptr<KCPClientInterface> impl_;
};
}

#endif // !_KCP_CLIENT_H_INCLUDED
