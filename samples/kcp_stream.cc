#include <iostream>
#include <random>

#include "kcp_interface.h"
#include "logging.h"

template<size_t N = 8 * 1024>
union TimestampBuf {
    struct {
        uint32_t timestamp;
        uint32_t seq;
    };

    char data[N];
};

class KCPStreamTestCallback : public kcp::KCPStreamCallback {
public:
    KCPStreamTestCallback(int id, kcp::KCPContextInterface* ctx, kcp::IP4Address& a, kcp::IP4Address& b, uint32_t conv)
        : id_(id)
        , stream_(ctx->CreateStream(a, b, conv)) {

        kcp::KCPConfig config;
        config.interval = 5;
        config.nocwnd = true;
        config.nodelay = true;
        config.resend = 2;

        stream_->Open(config, this);
    }

    bool OnError(const std::error_code& ec) override {
        return false;
    }
protected:
    int id_;
    std::unique_ptr<kcp::KCPStreamInterface> stream_;
};

class KCPStreamSendCallback : public KCPStreamTestCallback {
public:
    using KCPStreamTestCallback::KCPStreamTestCallback;

    void Test() {
        TimestampBuf<> buf;
        buf.timestamp = kcp::Now32();
        buf.seq = send_seq_++;
        stream_->Write(buf.data, sizeof buf);
    }

    virtual void OnRecvKCP(const char *buf, size_t size) {
        auto tb = reinterpret_cast<const TimestampBuf<>*>(buf);
        //assert(tb->seq == recv_seq_++);

        std::clog << "id=" << id_ 
            << ",ttl=" << kcp::Now32() - tb->timestamp << std::endl;

        Test();
    }

private:
    uint32_t send_seq_ = 0;
    uint32_t recv_seq_ = 0;
};

class KCPStreamRecvCallback : public KCPStreamTestCallback {
public:
    using KCPStreamTestCallback::KCPStreamTestCallback;

    virtual void OnRecvKCP(const char *buf, size_t size) {
        stream_->Write(buf, size);
    }
};

class KCPStreamTest {
public:
    KCPStreamTest(int id, kcp::KCPContextInterface* ctx, kcp::IP4Address& a, kcp::IP4Address& b, uint32_t conv)
        : sender_(id, ctx, a, b, conv)
        , recver_(id, ctx, b, a, conv) {}

    void Test() {
        sender_.Test();
    }
private:
    KCPStreamSendCallback sender_;
    KCPStreamRecvCallback recver_;
};

void Test() {
    auto ctx = kcp::KCPContextInterface::Create();
    ctx->Start();

    std::vector<std::thread> tg;

    for (int i = 0; i < 1; ++i) {
        tg.emplace_back([&ctx, i] {
            kcp::IP4Address a{ "127.0.0.1", (uint16_t)(4455 + i) };
            kcp::IP4Address b{ "127.0.0.1", (uint16_t)(4456 + i) };

            KCPStreamTest test(i, ctx.get(), a, b, i);
            test.Test();

            std::default_random_engine e;
            std::uniform_int<size_t> u(1, 60);

            std::this_thread::sleep_for(std::chrono::seconds(u(e)));
        });
    }

    for (auto&& t : tg) {
        t.join();
    }
}

int main() {
    for (int i = 0; i < 64; ++i) {
        Test();
    }
}