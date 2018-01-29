#include "kcp_proxy.h"
#include "kcp_stream.h"

using namespace kcp;

class KCPMux::UDPAdapter : public UDPInterface {
public:
    UDPAdapter(std::shared_ptr<KCPMux> mux, const StreamKey& key)
        : mux_(mux)
        , key_(key) {}

    ~UDPAdapter() {
        mux_->by_key_streams_.erase(key_);
        mux_.reset();
    }

    bool Open(UDPCallback *cb) override {
        cb_ = cb;
        return true;
    }

    void Close() override {
        cb_ = nullptr;
    }

    void SetRecvBufSize(size_t recv_size) override {
        mux_->SetRecvBufSize(recv_size);
    }

    bool Send(const IP4Address& to, const char *buf, size_t len) override {
        return mux_->Send(to, buf, len);
    }

    const IP4Address& local_address() const override {
        return  mux_->local_address();
    }

    ExecutorInterface *executor() override {
        return  mux_->executor();
    }
private:
    friend class KCPMux;

    std::shared_ptr<KCPMux> mux_;
    StreamKey key_;
    UDPCallback *cb_ = nullptr;
};

// static
std::shared_ptr<KCPMux>
KCPMux::Create(std::shared_ptr<UDPInterface> udp, bool bind_callback) {
    std::shared_ptr<KCPMux> mux{ new KCPMux(udp), [](KCPMux *p) { delete p; } };
    if (mux && bind_callback && !mux->BindUDPCallback()) {
        return nullptr;
    }

    return mux;
}

KCPMux::~KCPMux() {
    if (udp_callback_binded_ && udp_) {
        udp_->Close();
    }
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

bool KCPMux::BindUDPCallback() {
    if (!udp_->Open(this)) {
        return false;
    }

    udp_callback_binded_ = true;
    return true;
}

bool KCPMux::Open(UDPCallback *cb) {
    return true;
}

void KCPMux::Close() {}

void KCPMux::SetRecvBufSize(size_t recv_size) {
    udp_->SetRecvBufSize(recv_size);
}

bool KCPMux::Send(const IP4Address& to, const char *buf, size_t len) {
    return udp_->Send(to, buf, len);
}

const IP4Address& KCPMux::local_address() const {
    return udp_->local_address();
}

ExecutorInterface *KCPMux::executor() {
    return udp_->executor();
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

    auto new_mux = udp->executor()->Invoke(&KCPMux::Create, udp, true);
    if (new_mux) {
        by_address_muxes_.insert_or_assign(addr, new_mux);
    }

    return new_mux;
}

std::shared_ptr<UDPInterface>
KCPProxy::AddUDPFilter(IOContextInterface *io_ctx,
                       const IP4Address& addr,
                       const IP4Address& peer,
                       uint32_t conv) {
    auto mux = AddMux(io_ctx, addr);
    if (!mux) {

        return nullptr;
    }

    StreamKey key(peer, conv);
    return mux->AddUDPFilter(key);
}

bool KCPProxy::IsMuxAddress(const IP4Address& addr) const {
    return by_address_muxes_.end() != by_address_muxes_.find(addr);
}
