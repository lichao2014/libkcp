#include "kcp_proxy.h"

using namespace kcp;

class KCPProxy::UDPAdapter : public UDPInterface {
public:
    UDPAdapter(std::shared_ptr<KCPProxy> proxy,
               const UDPAddress& key)
        : proxy_(proxy)
        , key_(key) {}

    ~UDPAdapter() {
        proxy_->by_key_streams_.erase(key_);
        proxy_.reset();
    }

    bool Open(size_t recv_size, UDPCallback *cb) override {
        cb_ = cb;
        return true;
    }

    void Close() override {
        cb_ = nullptr;
    }

    bool Send(const UDPAddress& to, const char *buf, size_t len) override {
        return proxy_->udp_->Send(to, buf, len);
    }

    const UDPAddress& local_address() const override {
        return  proxy_->udp_->local_address();
    }

    ExecutorInterface *executor() override {
        if (executor_) {
            return executor_;
        }

        return  proxy_->udp_->executor();
    }
private:
    friend class KCPProxy;

    std::shared_ptr<KCPProxy> proxy_;
    UDPAddress key_;
    UDPCallback *cb_ = nullptr;
    ExecutorInterface *executor_ = nullptr;
};

//static 
std::shared_ptr<KCPProxy> 
KCPProxy::Create(std::shared_ptr<UDPInterface> udp) {
    return { new KCPProxy(udp), [](KCPProxy *p) { delete p; } };
}

std::shared_ptr<UDPInterface>
KCPProxy::AddFilter(const StreamKey& key) {
    if (by_key_streams_.find(key) != by_key_streams_.end()) {
        return nullptr;
    }

    auto udp = std::make_shared<UDPAdapter>(shared_from_this(), key);
    if (udp) {
        by_key_streams_.emplace(key, udp.get());
    }

    return udp;
}

KCPProxy::Result
KCPProxy::RecvUDP(const UDPAddress& from, const char *buf, size_t len, uint32_t *conv) {
    if (!ParseConv(buf, len, conv)) {
        return kBadMsg;
    }

    UDPAddress key(from, *conv);
    auto by_key = by_key_streams_.find(key);
    if (by_key_streams_.end() != by_key) {
        by_key->second->cb_->OnRecvUDP(from, buf, len);
        return kSuccess;
    }

    return kNotFound;
}

//static 
bool KCPProxy::ParseConv(const char *buf, size_t len, uint32_t *conv) {
    // conv is le32
    if (len < 4) {
        return false;
    }

    if (conv) {
        *conv = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    }

    return true;
}
