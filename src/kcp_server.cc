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

    bool Open(UDPCallback *cb) override {
        executor()->Invoke([&] {
            cb_ = cb;
        });

        return true;
    }

    void Close() override {
        executor()->Invoke([this] {
            cb_ = nullptr;
        });
    }

    bool Send(const UDPAddress& to, const char *buf, size_t len) override {
        return executor()->Invoke(&UDPInterface::Send, impl_, to, buf, len);
    }

    void SetRecvBufSize(size_t recv_size) override {

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
KCPServer::Create(std::shared_ptr<UDPInterface> udp) {
    return { new KCPServer(udp), [](KCPServer *p) { delete p; } };
}

KCPServer::~KCPServer() {
    Stop();
}

bool KCPServer::Start(KCPServerCallback *cb) {
    cb_ = cb;
    stopped_ = false;
    udp_->SetRecvBufSize(kEthMTU);
    return udp_->Open(this);
}

void KCPServer::Stop() {
    if (stopped_) {
        return;
    }

    stopped_ = true;

    if (udp_) {
        udp_->Close();
    }

    clients_.clear();
}

const UDPAddress& KCPServer::local_address() const {
    return udp_->local_address();
}

void KCPServer::OnRecvUdp(const UDPAddress& from, const char *buf, size_t len) {
    if (len < 4) {
        return;
    }

    uint32_t conv = ParseConv(buf);

    UDPAddress key(from, conv);
    auto it = clients_.find(key);
    if (clients_.end() == it) {
        auto adapter = std::make_shared<UDPAdapter>(udp_, shared_from_this(), key);
        if (!cb_->OnAccept(KCPClient::Create(adapter), from, conv)) {
            return;
        }

        it = clients_.emplace(key, adapter.get()).first;
    }

    it->second->cb_->OnRecvUdp(from, buf, len);
}

bool KCPServer::OnError(int err, const char *what) {
    return true;
}
