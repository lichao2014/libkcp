#include "kcp_server.h"
#include "kcp_client.h"


using namespace kcp;

bool KCPServer::Start(KCPServerCallback *cb) {
    if (!udp_->Open(config_.recv_size, this)) {
        return false;
    }

    cb_ = cb;
    stopped_ = false;
    proxy_ = KCPMux::Create(udp_);

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

const IP4Address& KCPServer::local_address() const {
    return udp_->local_address();
}

ExecutorInterface *KCPServer::executor() {
    return udp_->executor();
}

bool KCPServer::OnRecvUDP(const IP4Address& from, const char *buf, size_t len) {
    if (stopped_) {
        return false;
    }

    KCP_ASSERT(proxy_);
    return proxy_->OnRecvUDP(from, buf, len);
}

bool KCPServer::OnNewClient(const IP4Address& from,
                            uint32_t conv,
                            const char *buf,
                            size_t len) {
    if (!cb_) {
        return false;
    }

    IP4Address key(from, conv);
    auto udp = proxy_->AddUDPFilter(key);
    if (!udp) {
        return false;
    }

    auto client = KCPClientAdapter::Create(udp, from, config_);
    if (!client) {
        return false;
    }

    if (!cb_->OnAccept(std::move(client))) {
        return false;
    }

    return proxy_->OnRecvUDP(from, buf, len);
}

bool KCPServer::OnError(const std::error_code& ec) {
    if (!cb_) {
        return true;
    }

    return cb_->OnError(ec);
}

//static
std::unique_ptr<KCPServerAdapter>
KCPServerAdapter::Create(std::shared_ptr<UDPInterface> udp,
                         const KCPServerConfig& config) {
    auto server = std::make_unique<KCPServer>(udp, config);
    if (!server) {
        return nullptr;
    }

    return std::make_unique<KCPServerAdapter>(std::move(server));
}
