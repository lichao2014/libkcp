#include <iostream>

#include "kcp_interface.h"


class KCPServerCallback : public kcp::KCPServerCallback, public kcp::KCPClientCallback {
public:
    bool OnAccept(std::shared_ptr<kcp::KCPClientInterface> client, const kcp::UDPAddress& from, uint32_t conv) override {
        client_ = client;
        std::clog << "incoming a client from=" << from.ip4_string() << ',' << from.port << ",conv=" << conv << std::endl;
        return client->Connect(from, conv, {}, this);
    }

    void OnRecvKCP(const char *buf, size_t size) override {
        static int cnt = 0;
        std::clog << "recv kcp:cnt=" << cnt++ << std::endl;

        client_->Write(buf, size);
    }

    bool OnError(const std::error_code& ec) override {
        std::cerr << "OnError:ec=" << ec.category().name() << ',' << ec.message() << std::endl;
        return false;
    }

    std::shared_ptr<kcp::KCPClientInterface> client_;
};

int main() {
    KCPServerCallback server_callback;

    auto ctx = kcp::KCPContextInterface::Create();
    ctx->Start();

    auto s = ctx->CreateServer({ "127.0.0.1", 1234 });
    s->Start(&server_callback);

    std::cin.get();
}