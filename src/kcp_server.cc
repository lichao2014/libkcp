#include "kcp_server.h"
#include "kcp_client.h"

using namespace kcp;

//static 
std::shared_ptr<KCPServer> 
KCPServer::Create(std::shared_ptr<UDPInterface> udp) {
    return { new KCPServer(udp), [](KCPServer *p) { delete p; } };
}

KCPServer::~KCPServer() {
    Stop();
}

bool KCPServer::Start(KCPServerCallback *cb) {
    if (!udp_->Open(1024 * 64, this)) {
        return false;
    }

    cb_ = cb;
    stopped_ = false;
    proxy_ = KCPProxy::Create(udp_);

    return true;
}

void KCPServer::Stop() {
    if (stopped_) {
        return;
    }

    stopped_ = true;

    if (udp_) {
        udp_->Close();
    }
}

const UDPAddress& KCPServer::local_address() const {
    return udp_->local_address();
}

void KCPServer::OnRecvUDP(const UDPAddress& from, const char *buf, size_t len) {
    if (stopped_) {
        return;
    }

    assert(proxy_);

    uint32_t conv;
    switch (proxy_->RecvUDP(from, buf, len, &conv)) {
    case KCPProxy::kNotFound:
        OnNewClient(from, conv, buf, len);
        break;
    case KCPProxy::kSuccess:
        break;
    case KCPProxy::kBadMsg:
    default:
        OnError(MakeErrorCode(ErrNum::kBadUDPMsg));
        break;
    }
}

void KCPServer::OnNewClient(const UDPAddress& from, 
                            uint32_t conv, 
                            const char *buf, 
                            size_t len) {
    if (!cb_) {
        return;
    }

    UDPAddress key(from, conv);
    auto udp = proxy_->AddFilter(key);
    if (!udp) {
        return;
    }

    auto client = std::make_shared<KCPClientAdapter>(KCPClient::Create(udp));
    if (!cb_->OnAccept(client, from, conv)) {
        return;
    }

    proxy_->RecvUDP(from, buf, len, &conv);
}

bool KCPServer::OnError(const std::error_code& ec) {
    if (!cb_) {
        return true;
    }

    return cb_->OnError(ec);
}

