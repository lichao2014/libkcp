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

std::shared_ptr<KCPClientInterface> 
KCPContext::CreateClient(const UDPAddress& addr) {
    auto udp = io_ctx_->CreateUDP(addr);
    if (!udp) {
        return nullptr;
    }

    auto client = udp->executor()->Invoke(KCPClient::Create, udp);
    if (!client) {
        return nullptr;
    }

    return std::make_shared<KCPClientAdapter>(client);
}

std::shared_ptr<KCPServerInterface> 
KCPContext::CreateServer(const UDPAddress& addr) {
    auto udp = io_ctx_->CreateUDP(addr);
    if (!udp) {
        return nullptr;
    }

    auto server = udp->executor()->Invoke(KCPServer::Create, udp);
    if (!server) {
        return nullptr;
    }

    return std::make_shared<KCPServerAdapter>(server);
}
