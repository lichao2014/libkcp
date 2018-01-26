#include "kcp_client.h"

using namespace kcp;

bool KCPClient::Connect(const UDPAddress& to,
                        uint32_t conv,
                        const KCPConfig& config,
                        KCPClientCallback *cb) {
    stream_ = std::make_unique<KCPStream>(udp_, to, conv, config, cb);
    if (!stream_) {
        return false;
    }

    return stream_->Open();
}

bool KCPClient::Write(const char *buf, std::size_t len) {
    return stream_->Write(buf, len);
}

void KCPClient::Close() {
    stream_->Close();
}

const UDPAddress& KCPClient::local_address() const {
    return udp_->local_address();
}

const UDPAddress& KCPClient::remote_address() const {
    return stream_->peer();
}

uint32_t KCPClient::conv() const {
    return stream_->conv();
}

ExecutorInterface *KCPClient::executor() {
    return udp_->executor();
}

//static
std::shared_ptr<KCPClientAdapter>
KCPClientAdapter::Create(std::shared_ptr<UDPInterface> udp) {
    return {
        new KCPClientAdapter(std::make_unique<KCPClient>(udp)),
        [] (KCPClientAdapter *p) { delete p; }
    };
}
