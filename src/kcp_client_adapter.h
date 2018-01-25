#ifndef _KCP_CLIENT_ADAPTER_H_INCLUDED
#define _KCP_CLIENT_ADAPTER_H_INCLUDED

#include "kcp_client.h"

namespace kcp {
class KCPClientAdapter : public KCPClientInterface {
public:
    explicit KCPClientAdapter(std::shared_ptr<KCPClient> impl) : impl_(impl) {}

    ~KCPClientAdapter() {
        executor()->Invoke([&] {
            impl_.reset();
        });
    }

    bool Connect(const UDPAddress& to, uint32_t conv, const KCPConfig& config, KCPClientCallback *cb) override {
        return executor()->Invoke(&KCPClient::Connect, impl_, to, conv, config, cb);
    }

    bool Write(const char *buf, std::size_t len) {
        return executor()->Invoke(&KCPClient::Write, impl_, buf, len);
    }

    void Close() override {
        executor()->Invoke(&KCPClient::Close, impl_);
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
    std::shared_ptr<KCPClient> impl_;
};
}

#endif // !_KCP_CLIENT_ADAPTER_H_INCLUDED
