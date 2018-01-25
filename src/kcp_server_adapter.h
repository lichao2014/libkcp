#ifndef _KCP_SERVER_ADAPTER_H_INCLUDED
#define _KCP_SERVER_ADAPTER_H_INCLUDED

#include "kcp_server.h"

namespace kcp {
class KCPServerAdapter : public KCPServerInterface {
public:
    explicit KCPServerAdapter(std::shared_ptr<KCPServer> impl) : impl_(impl) {}

    ~KCPServerAdapter() {
        executor()->Invoke([this] {
            impl_.reset();
        });
    }

    bool Start(KCPServerCallback *cb) override {
        return executor()->Invoke(&KCPServer::Start, impl_, cb);
    }

    void Stop() override {
        executor()->Invoke(&KCPServer::Stop, impl_);
    }

    const UDPAddress& local_address() const override {
        return impl_->local_address();
    }

    ExecutorInterface *executor() override {
        return impl_->executor();
    }
private:
    std::shared_ptr<KCPServer> impl_;
};
}

#endif // !_KCP_SERVER_ADAPTER_H_INCLUDED
