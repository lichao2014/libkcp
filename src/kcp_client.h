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

class KCPClientAdapter : public KCPClientInterface {
public:
    explicit KCPClientAdapter(std::shared_ptr<KCPClientInterface> impl) 
        : impl_(impl) {}

    ~KCPClientAdapter() {
        executor()->Invoke([&] {
            impl_->Close();
            impl_.reset();
        });
    }

    bool Connect(const UDPAddress& to,
                uint32_t conv,
                const KCPConfig& config,
                KCPClientCallback *cb) override {
        return executor()->Invoke(
            &KCPClientInterface::Connect,impl_, to, conv, config, cb);
    }

    bool Write(const char *buf, std::size_t len) {
        return executor()->Invoke(&KCPClientInterface::Write, impl_, buf, len);
    }

    void Close() override {
        executor()->Invoke(&KCPClientInterface::Close, impl_);
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
    std::shared_ptr<KCPClientInterface> impl_;
};
}

#endif // !_KCP_CLIENT_H_INCLUDED
