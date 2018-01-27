#include <iostream>

#include "kcp_interface.h"
#include "kcp_log.h"

class KCPStreamTestCallback : public kcp::KCPStreamCallback {
public:
    virtual void OnRecvKCP(const char *buf, size_t size) {

    }

    virtual bool OnError(const std::error_code& ec) {
        return false;
    }
};

int main() {
    using namespace kcp;

    LogMessage::SetEnabled(true);

    auto ctx = KCPContextInterface::Create();
    ctx->Start();

    IP4Address a{ "127.0.0.1", 3445 };
    IP4Address b{ "127.0.0.1", 3446 };

    auto s1 = ctx->CreateStream(a, b, 1);
    auto s2 = ctx->CreateStream(b, a, 1);

    KCPStreamTestCallback cb1;
    KCPStreamTestCallback cb2;

    s1->Open({}, &cb1);
    s2->Open({}, &cb2);

    s1->Write("12", 2);

    std::cin.get();
}