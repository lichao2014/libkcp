#include "kcp_client.h"

using namespace kcp;

bool KCPClient::Open(KCPClientCallback *cb) {
    stream_ = std::make_unique<KCPStream>(udp_, peer_, 0);
    if (!stream_) {
        return false;
    }

    cb_ = cb;
    cb_->OnConnected(true);

    return stream_->Open(config_, this);
}

void KCPClient::Close() {
    stream_->Close();
}

bool KCPClient::Write(const char *buf, std::size_t len) {
    return stream_->Write(buf, len);
}

uint32_t KCPClient::conv() const {
    return stream_->conv();
}

const IP4Address& KCPClient::local_address() const {
    return udp_->local_address();
}

const IP4Address& KCPClient::remote_address() const {
    return stream_->remote_address();
}

ExecutorInterface *KCPClient::executor() {
    return udp_->executor();
}

void KCPClient::OnRecvKCP(const char *buf, size_t size) {
    cb_->OnRecvKCP(buf, size);
}

bool KCPClient::OnError(const std::error_code& ec) {
    return cb_->OnError(ec);
}

// static
std::unique_ptr<KCPClientAdapter>
KCPClientAdapter::Create(std::shared_ptr<UDPInterface> udp,
                         const IP4Address& peer,
                         const KCPConfig& config) {
    auto client = std::make_unique<KCPClient>(udp, peer, config);
    if (!client) {
        return nullptr;
    }

    return std::make_unique<KCPClientAdapter>(std::move(client));
}
