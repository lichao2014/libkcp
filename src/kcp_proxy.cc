#include "kcp_proxy.h"
#include "kcp_stream.h"

using namespace kcp;

class KCPMux::UDPAdapter : public UDPInterface {
public:
    UDPAdapter(std::shared_ptr<KCPMux> proxy, const StreamKey& key)
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

    bool Send(const IP4Address& to, const char *buf, size_t len) override {
        return proxy_->udp_->Send(to, buf, len);
    }

    const IP4Address& local_address() const override {
        return  proxy_->udp_->local_address();
    }

    ExecutorInterface *executor() override {
        return  proxy_->udp_->executor();
    }
private:
    friend class KCPMux;

    std::shared_ptr<KCPMux> proxy_;
    StreamKey key_;
    UDPCallback *cb_ = nullptr;
};

//static
std::shared_ptr<KCPMux>
KCPMux::Create(std::shared_ptr<UDPInterface> udp) {
    return { new KCPMux(udp), [](KCPMux *p) { delete p; } };
}

std::shared_ptr<UDPInterface>
KCPMux::AddUDPFilter(const StreamKey& key) {
    if (by_key_streams_.find(key) != by_key_streams_.end()) {
        return nullptr;
    }

    auto udp = std::make_shared<UDPAdapter>(shared_from_this(), key);
    if (udp) {
        by_key_streams_.emplace(key, udp.get());
    }

    return udp;
}

bool KCPMux::OnRecvUDP(const IP4Address& from, const char *buf, size_t len) {
    uint32_t conv = 0;
    if (!KCPAPI::ParseConv(buf, len, &conv)) {
        return false;
    }

    StreamKey key(from, conv);
    auto by_key = by_key_streams_.find(key);
    if (by_key_streams_.end() != by_key) {
        return by_key->second->cb_->OnRecvUDP(from, buf, len);
    }

    return false;
}

bool KCPMux::OnError(const std::error_code& ec) {
    for (auto&& stream : by_key_streams_) {
        if (!stream.second->cb_->OnError(ec)) {
            return false;
        }
    }

    return true;
}

std::shared_ptr<KCPMux>
KCPProxy::AddMux(IOContextInterface *io_ctx, const IP4Address& addr) {
    auto by_address = by_address_muxes_.find(addr);
    if (by_address_muxes_.end() != by_address) {
        auto mux = by_address->second.lock();
        if (mux) {
            return mux;
        }
    }

    auto udp = io_ctx->CreateUDP(addr);
    if (!udp) {
        return nullptr;
    }

    auto new_mux = KCPMux::Create(udp);
    if (new_mux) {
        udp->Open(1024 * 64, new_mux.get());
        by_address_muxes_.insert_or_assign(addr, new_mux);
    }

    return new_mux;
}

std::shared_ptr<UDPInterface>
KCPProxy::AddUDPFilter(IOContextInterface *io_ctx,const IP4Address& addr, uint32_t conv) {
    auto mux = AddMux(io_ctx, addr);
    if (!mux) {

        return nullptr;
    }

    StreamKey key(addr, conv);
    return mux->AddUDPFilter(key);
}

bool KCPProxy::IsMuxAddress(const IP4Address& addr) const {
    return by_address_muxes_.end() != by_address_muxes_.find(addr);
}
