#include <iostream>

#include "kcp_interface.h"


class KCPClientCallback : public kcp::KCPClientCallback {
public:
    KCPClientCallback(std::shared_ptr<kcp::KCPClientInterface> client) : client_(client) {}

    void OnRecvKCP(const char *buf, size_t size) override {
        static int cnt = 0;
        std::clog << "recv kcp:cnt=" << cnt++ << std::endl;

        client_->Write(buf, size);
    }

    bool OnError(const std::error_code& ec) override {
        std::cerr << "OnError:ec=" << ec.category().name() << ',' << ec.message() << std::endl;
        return false;
    }
private:
    std::shared_ptr<kcp::KCPClientInterface> client_;
};

class KCPServerCallback : public kcp::KCPServerCallback {
public:
    KCPServerCallback(kcp::KCPContextInterface *ctx, const kcp::UDPAddress& a)
        : server_(ctx->CreateServer(a)) {}

    void Start() {
        server_->Start(this);
    }

    bool OnAccept(std::shared_ptr<kcp::KCPClientInterface> client, const kcp::UDPAddress& from, uint32_t conv) override {
        clients_.emplace_back(client);
        std::clog << "incoming a client from=" << from.ip4_string() << ',' << from.port << ",conv=" << conv << std::endl;
        return client->Connect(from, conv, {}, &clients_.back());
    }

    bool OnError(const std::error_code& ec) override {
        std::cerr << "OnError:ec=" << ec.category().name() << ',' << ec.message() << std::endl;
        return false;
    }
private:
    std::shared_ptr<kcp::KCPServerInterface> server_;
    std::vector<KCPClientCallback> clients_;
};

int main() {
    auto ctx = kcp::KCPContextInterface::Create();
    ctx->Start();

    KCPServerCallback server(ctx.get(), { "127.0.0.1", 1234 });

    server.Start();

    std::cin.get();
}