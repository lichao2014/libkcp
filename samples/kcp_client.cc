#include <iostream>
#include <thread>
#include "boost/thread/future.hpp"

#include "kcp_interface.h"
#include "kcp_log.h"

class KCPClient : public kcp::KCPClientCallback {
public:
    template<size_t N>
    union TimestampBuf {
        uint32_t timestamp;
        char data[N];
    };

    explicit KCPClient(int id) : id_(id) {}

    void OnRecvKCP(const char *buf, size_t size) override {
        //std::clog << "kcp recv:len=" << size << std::endl;

        uint32_t now = kcp::Now32();
        auto p = reinterpret_cast<const TimestampBuf<1024 * 8> *>(buf);
        std::clog << "id=" << id_ << ",ttl=" << now - p->timestamp << std::endl;

        Test();
    }

    bool OnError(const std::error_code& ec) override {
        std::cerr << "OnError:ec=" << ec.category().name() 
            << ',' << ec.message() << std::endl;
        return false;
    }

    bool Init(kcp::KCPContextInterface *ctx, const kcp::IP4Address& a, const kcp::IP4Address& b, uint32_t conv) {
        client_ = ctx->CreateClient(a);
        return client_->Connect(b, conv, {}, this);
    }

    void Test() {
        buf_.timestamp = kcp::Now32();
        KCP_ASSERT(client_->Write(buf_.data, sizeof buf_));
    }

private:
    int id_;
    std::unique_ptr<kcp::KCPClientInterface> client_;
    TimestampBuf<1024 * 8> buf_;
};

int main() {
    kcp::LogMessage::SetEnabled(false);

    auto ctx = kcp::KCPContextInterface::Create();
    ctx->Start();

    std::vector<std::unique_ptr<KCPClient>> clients;

    for (int i = 0; i < 6; ++i) {
        auto client = std::make_unique<KCPClient>(i);
        std::thread([&ctx, p = client.get(), port = 1235 + i, i] () mutable {
            p->Init(ctx.get(), { "127.0.0.1", (uint16_t)port }, { "127.0.0.1", 1234 }, i);
            p->Test();
        }).detach();

        clients.emplace_back(std::move(client));
    }

    std::cin.get();
}