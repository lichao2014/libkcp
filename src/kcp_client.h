#ifndef _KCP_CLIENT_H_INCLUDED
#define _KCP_CLIENT_H_INCLUDED

#include <vector>

#include "kcp_interface.h"
#include "udp_interface.h"
#include "kcp_stream.h"
#include "kcp_error.h"

namespace kcp {
class KCPClient   {
public:
    explicit KCPClient(std::shared_ptr<UDPInterface> udp): udp_(udp) {}
    ~KCPClient() = default;

    bool Connect(const UDPAddress& to,
                 uint32_t conv,
                 const KCPConfig& config,
                 KCPClientCallback *cb);
    bool Write(const char *buf, std::size_t len);
    void Close();
    const UDPAddress& local_address() const;
    const UDPAddress& remote_address() const;
    uint32_t conv() const;
    ExecutorInterface *executor();
private:
    std::unique_ptr<KCPStream> stream_;
    std::shared_ptr<UDPInterface> udp_;
};

class KCPClientAdapter : public KCPClientInterface {
public:
    static std::shared_ptr<KCPClientAdapter> Create(std::shared_ptr<UDPInterface> udp);

    bool Connect(const UDPAddress& to,
                uint32_t conv,
                const KCPConfig& config,
                KCPClientCallback *cb) override {
        return executor()->Invoke(&KCPClient::Connect,
                                   impl_.get(),
                                   to,
                                   conv,
                                   config,
                                   cb);
    }

    bool Write(const char *buf, std::size_t len) {
        return executor()->Invoke(&KCPClient::Write, impl_.get(), buf, len);
    }

    void Close() override {
        executor()->Invoke(&KCPClient::Close, impl_.get());
    }

    const UDPAddress& local_address() const override {
        return impl_->local_address();
    }

    const UDPAddress& remote_address() const override {
        return impl_->remote_address();
    }

    uint32_t conv() const override {
        return impl_->conv();
    }

    ExecutorInterface *executor() override {
        return impl_->executor();
    }
private:
    explicit KCPClientAdapter(std::unique_ptr<KCPClient> impl)
        : impl_(std::move(impl)) {}

    ~KCPClientAdapter() {
        executor()->Invoke([this] {
            impl_->Close();
            impl_.reset();
        });
    }

    std::unique_ptr<KCPClient> impl_;
};
}

#endif // !_KCP_CLIENT_H_INCLUDED
