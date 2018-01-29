#include "kcp_context.h"
#include "kcp_client.h"
#include "kcp_server.h"

using namespace kcp;

//static
std::unique_ptr<KCPContextInterface>
KCPContextInterface::Create(size_t thread_num) {
    return std::make_unique<KCPContext>(IOContextInterface::Create(thread_num));
}

void KCPContext::Start() {
    io_ctx_->Start();
}

void KCPContext::Stop() {
    io_ctx_->Stop();
}

ExecutorInterface *KCPContext::executor() {
    return io_ctx_->executor();
}

std::unique_ptr<KCPStreamInterface>
KCPContext::CreateStream(const IP4Address& addr,
                         const IP4Address& peer,
                         uint32_t conv) {
    auto udp = proxy_->AddUDPFilter(io_ctx_.get(), addr, peer, conv);
    if (!udp) {
        return nullptr;
    }

    return KCPStreamAdapter::Create(udp, peer, conv);
}

std::unique_ptr<KCPClientInterface>
KCPContext::CreateClient(const IP4Address& addr,
                        const IP4Address& peer,
                        const KCPConfig& config) {
    auto udp = io_ctx_->CreateUDP(addr);
    if (!udp) {
        return nullptr;
    }

    return KCPClientAdapter::Create(udp, peer, config);
}

std::unique_ptr<KCPServerInterface>
KCPContext::CreateServer(const IP4Address& addr,
                         const KCPServerConfig& config) {
    auto udp = io_ctx_->CreateUDP(addr);
    if (!udp) {
        return nullptr;
    }

    return KCPServerAdapter::Create(udp, config);
}
