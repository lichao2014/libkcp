#include "kcp_server.h"
#include "kcp_client.h"

using namespace kcp;

namespace {
uint32_t ParseConv(const char buf[4]) {
    return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}
}

class KCPServer::UDPAdapter : public UDPInterface {
public:
    UDPAdapter(std::shared_ptr<UDPInterface> impl, std::shared_ptr<KCPServer> server, const UDPAddress& key)
        : impl_(impl)
        , server_(server)
        , key_(key) {}

    ~UDPAdapter() {
        executor()->Invoke([this] {
            server_->clients_.erase(key_);

            server_.reset();
            impl_.reset();
        });
    }

    void Start(UDPCallback *cb) override {
        executor()->Invoke([&] {
            cb_ = cb;
        });
    }

    void Stop() override {
        executor()->Invoke([this] {
            cb_ = nullptr;
        });
    }

    bool Send(const UDPAddress& to, const char *buf, size_t len) override {
        return executor()->Invoke(&UDPInterface::Send, impl_, to, buf, len);
    }

    const UDPAddress& local_address() const override {
        return impl_->local_address();
    }

    ExecutorInterface *executor() override { return impl_->executor(); }
private:
    friend class KCPServer;

    std::shared_ptr<UDPInterface> impl_;
    std::shared_ptr<KCPServer> server_;
    UDPAddress key_;
    UDPCallback *cb_ = nullptr;
};

//static 
std::shared_ptr<KCPServer> 
KCPServer::Create(std::shared_ptr<UDPInterface> udp, IOContextInterface& io_ctx) {
    return { new KCPServer(udp, io_ctx), [](KCPServer *p) { delete p; } };
}

bool KCPServer::Start(KCPServerCallback *cb) {
    cb_ = cb;
    udp_->Start(this);
    return true;
}

void KCPServer::Stop() {
    if (udp_) {
        udp_->Stop();
    }
}

const UDPAddress& KCPServer::local_address() const {
    return udp_->local_address();
}

void KCPServer::OnRecvUdp(const UDPAddress& from, const char *buf, size_t len) {
    if (len < 4) {
        return;
    }

    uint32_t conv = ParseConv(buf);

    UDPAddress key;
    key.u64 = from.u64;
    key.conv = static_cast<uint16_t>(conv);

    auto it = clients_.find(key);
    if (clients_.end() == it) {
        auto adapter = std::make_shared<UDPAdapter>(udp_, shared_from_this(), key);
        if (!cb_->OnAccept(KCPClient::Create(adapter, io_ctx_), from, conv)) {
            return;
        }

        it = clients_.emplace(key, adapter.get()).first;
    }

    it->second->cb_->OnRecvUdp(from, buf, len);
}

bool KCPServer::OnError(int err, const char *what) {
    return true;
}

