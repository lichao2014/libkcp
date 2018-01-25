#include <iostream>
#include <cassert>

#include "kcp_interface.h"


class KCPClientCallback : public kcp::KCPClientCallback {
public:
    void OnRecvKCP(const char *buf, size_t size) override {
    }

    void OnError(int err, const char *what) override {
    }
};

class KCPServerCallback : public kcp::KCPServerCallback, public kcp::KCPClientCallback {
public:
    bool OnAccept(std::shared_ptr<kcp::KCPClientInterface> client, const kcp::UDPAddress& from, uint32_t conv) override {
        client_ = client;
        return client->Connect(from, conv, {}, this);
    }

    void OnRecvKCP(const char *buf, size_t size) override {
        static int cnt = 0;
        std::clog << cnt++ << std::endl;
    }

    void OnError(int err, const char *what) override {
    }

    std::shared_ptr<kcp::KCPClientInterface> client_;
};


int main() {
    kcp::UDPAddress a1("127.0.0.1", 1234);
    kcp::UDPAddress a2("127.0.0.1", 1235);

    KCPServerCallback server_callback;
    KCPClientCallback client_callback;

    auto ctx = kcp::KCPContextInterface::Create();
    ctx->Start();

    auto s = ctx->CreateServer(a1);
    s->Start(&server_callback);

    auto c = ctx->CreateClient(a2);
    c->Connect(a1, 0, {}, &client_callback);

    int i = 0;
    while (i < 1) {
        assert(c->Write("123456", 6));
        i++;
    }

    std::cin.get();

    //c->Close();
    //s->Stop();

    //ctx->Stop();
}