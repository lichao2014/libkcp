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

    bool Init(kcp::KCPContextInterface *ctx, const kcp::UDPAddress& a, const kcp::UDPAddress& b) {
        client_ = ctx->CreateClient(a);
        return client_->Connect(b, 0, {}, this);
    }

    void Test() {
        buf_.timestamp = kcp::Now32();
        assert(client_->Write(buf_.data, sizeof buf_));
    }

private:
    std::shared_ptr<kcp::KCPClientInterface> client_;

    TimestampBuf<1024 * 8> buf_;
};

int main() {
 
    auto ctx = kcp::KCPContextInterface::Create();
    ctx->Start();

    KCPClientCallback client;
    client.Init(ctx.get(), { "127.0.0.1", 1235 }, { "127.0.0.1", 1234 });

    client.Test();

    std::cin.get();
}