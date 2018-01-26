#include <iostream>
#include <cassert>

#include "kcp_interface.h"

class KCPClientCallback : public kcp::KCPClientCallback {
public:
    template<size_t N>
    union TimestampBuf {
        uint32_t timestamp;
        char data[N];
    };

    void OnRecvKCP(const char *buf, size_t size) override {
        //std::clog << "kcp recv:len=" << size << std::endl;

        uint32_t now = kcp::Now32();
        auto p = reinterpret_cast<const TimestampBuf<1024 * 8> *>(buf);
        std::clog << now - p->timestamp << std::endl;

        Test();
    }

    bool OnError(const std::error_code& ec) override {
        std::cerr << "OnError:ec=" << ec.category().name() 
            << ',' << ec.message() << std::endl;
        return false;
    }

    bool Init(kcp::KCPContextInterface *ctx, const kcp::UDPAddress& a, const kcp::UDPAddress& b, uint32_t conv) {
        client_ = ctx->CreateClient(a);
        return client_->Connect(b, conv, {}, this);
    }

    void Test() {
        buf_.timestamp = kcp::Now32();
        KCP_ASSERT(client_->Write(buf_.data, sizeof buf_));
    }

private:
    std::shared_ptr<kcp::KCPClientInterface> client_;

    TimestampBuf<1024 * 8> buf_;
};

int main() {
    auto ctx = kcp::KCPContextInterface::Create();
    ctx->Start();

    KCPClientCallback client1;
    client1.Init(ctx.get(), { "127.0.0.1", 1235 }, { "127.0.0.1", 1234 }, 0);
    client1.Test();

    KCPClientCallback client2;
    client2.Init(ctx.get(), { "127.0.0.1", 1236 }, { "127.0.0.1", 1234 }, 1);
    client2.Test();

    std::cin.get();
}