#include "kcp_client.h"

using namespace kcp;

//static 
std::shared_ptr<KCPClient> 
KCPClient::Create(std::shared_ptr<UDPInterface> udp) {
    return { new KCPClient(udp), [](KCPClient *p) { delete p; } };
}

bool KCPClient::Connect(const UDPAddress& to,
                        uint32_t conv,
                        const KCPConfig& config,
                        KCPClientCallback *cb) {
    stream_ = KCPStream::Create(udp_, to, conv, config);
    if (!stream_) {
        return false;
    }

    return stream_->Open(cb);
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