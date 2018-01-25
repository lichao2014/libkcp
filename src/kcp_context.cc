#include "kcp_context.h"
#include "kcp_client_adapter.h"
#include "kcp_server_adapter.h"

using namespace kcp;

//static 
std::unique_ptr<KCPContextInterface> KCPContextInterface::Create() {
    return std::make_unique<KCPContext>();
}

KCPContext::KCPContext()
    : io_ctx_(IOContextInterface::Create(1)){
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

std::shared_ptr<KCPClientInterface> 
KCPContext::CreateClient(const UDPAddress& addr) {
    auto udp = io_ctx_->CreateUDP(addr);
    if (!udp) {
        return nullptr;
    }

    auto client = udp->executor()->Invoke(KCPClient::Create, udp, *io_ctx_);
    if (!client) {
        return nullptr;
    }

    return std::make_unique<KCPClientAdapter>(client);
}

std::shared_ptr<KCPServerInterface> 
KCPContext::CreateServer(const UDPAddress& addr) {
    auto udp = io_ctx_->CreateUDP(addr);
    if (!udp) {
        return nullptr;
    }

    auto server = udp->executor()->Invoke(KCPServer::Create, udp, *io_ctx_);
    if (!server) {
        return nullptr;
    }

    return std::make_shared<KCPServerAdapter>(server);
}
