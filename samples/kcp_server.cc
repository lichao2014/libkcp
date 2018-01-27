#include <iostream>

#include "kcp_interface.h"
#include "kcp_log.h"

class KCPClient : public kcp::KCPClientCallback {
public:
    void set_client(std::unique_ptr<kcp::KCPClientInterface> client) {
        client_ = std::move(client);
    }

    void OnRecvKCP(const char *buf, size_t size) override {
        static int cnt = 0;
        std::clog << "****recv kcp:cnt=" << cnt++ << std::endl;

        client_->Write(buf, size);
    }

    bool OnError(const std::error_code& ec) override {
        std::cerr << "OnError:ec=" << ec.category().name() << ',' << ec.message() << std::endl;
        return false;
    }
private:
    std::unique_ptr<kcp::KCPClientInterface> client_;
};

class KCPServer : public kcp::KCPServerCallback {
public:
    KCPServer(kcp::KCPContextInterface *ctx, const kcp::IP4Address& a)
        : server_(ctx->CreateServer(a)) {}

    void Start() {
        server_->Start(this);
    }

    bool OnAccept(std::unique_ptr<kcp::KCPClientInterface> client, 
                const kcp::IP4Address& from, 
                uint32_t conv) override {
        std::clog << "incoming a client from=" << from.ip4_string() << ',' << from.port << ",conv=" << conv << std::endl;

        auto xclient = std::make_unique<KCPClient>();
        if (!client->Connect(from, conv, {}, xclient.get())) {
            std::cerr << "connect failed" << std::endl;
            return false;
        }

        xclient->set_client(std::move(client));
        clients_.emplace_back(std::move(xclient));

        return true;
    }

    bool OnError(const std::error_code& ec) override {
        std::cerr << "OnError:ec=" << ec.category().name() << ',' << ec.message() << std::endl;
        return false;
    }
private:
    std::shared_ptr<kcp::KCPServerInterface> server_;
    std::vector<std::unique_ptr<KCPClient>> clients_;
};

int main() {
    kcp::LogMessage::SetEnabled(false);

    auto ctx = kcp::KCPContextInterface::Create();
    ctx->Start();

    KCPServer server(ctx.get(), { "127.0.0.1", 1234 });

    server.Start();

    std::cin.get();
}