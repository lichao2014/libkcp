#ifndef _KCP_STREAM_H_INCLUDED
#define _KCP_STREAM_H_INCLUDED

#include <memory>
#include <cstdint>

#include "ikcp.h"

#include "udp_interface.h"
#include "kcp_interface.h"
#include "kcp_error.h"

namespace kcp {
class KCPAPI : public std::unique_ptr<ikcpcb, void (*)(ikcpcb *)> {
public:
    using OutputFunc = decltype(std::declval<ikcpcb>().output);

    KCPAPI()
        : std::unique_ptr<ikcpcb, void(*)(ikcpcb *)>(nullptr, &ikcp_release) {}

    // return recv size
    int Recv(char *buffer, int len) noexcept {
        return 0 == ikcp_recv(get(), buffer, len);
    }

    bool Send(const char *buffer, size_t len) noexcept {
        return 0 == ikcp_send(get(), buffer,static_cast<int>(len));
    }

    bool Input(const char *data, size_t size) noexcept {
        return 0 == ikcp_input(get(), data, static_cast<long>(size));
    }

    void Flush() noexcept {
        ikcp_flush(get());
    }

    // -1 failed
    int peek_size() const noexcept {
        return ikcp_peeksize(get());
    }

    // 0 success
    // else failed
    bool set_mtu(int mtu) noexcept {
        return 0 == ikcp_setmtu(get(), mtu);
    }

    // < 0 failed
    bool set_wndsize(int sndwnd, int rcvwnd) noexcept {
        return 0 == ikcp_wndsize(get(), sndwnd, rcvwnd);
    }

    // < 0 failed
    bool set_nodelay(int nodelay, int interval, int resend, int nc) noexcept {
        return 0 == ikcp_nodelay(get(), nodelay, interval, resend, nc);
    }

    void Update(uint32_t current) noexcept {
        ikcp_update(get(), current);
    }

    // never fail
    uint32_t Check(uint32_t current) const noexcept {
        return ikcp_check(get(), current);
    }

    bool Init(uint32_t conv, OutputFunc output, void *user) noexcept {
        auto kcp = ikcp_create(conv, user);
        if (kcp) {
            kcp->output = output;
            reset(kcp);
        }

        return kcp;
    }

    static bool ParseConv(const char *buf, size_t len, uint32_t *conv);
};

class KCPStream : public UDPCallback
                , public TaskInterface
                , public KCPStreamInterface {
public:
    KCPStream(std::shared_ptr<UDPInterface> udp,
              const IP4Address& peer,
              uint32_t conv)
        : udp_(udp)
        , peer_(peer)
        , conv_(conv) {}

    ~KCPStream() { Close(); }

    bool Open(const KCPConfig& config, KCPStreamCallback *cb) override;
    void Close() override;
    bool Write(const char *buf, size_t len) override;
    const IP4Address& local_address() const override;
    const IP4Address& remote_address() const override;
    uint32_t conv() const override;
    ExecutorInterface *executor() override;
private:
    void WriteUDP(const char *buf, std::size_t len);
    void TryRecvKCP();
    bool OnKCPError(ErrNum err);

    // udp callback
    bool OnRecvUDP(const IP4Address& from, const char *buf, size_t len) override;
    bool OnError(const std::error_code& ec) override;

    // task callback
    uint32_t OnRun(uint32_t now) override;
    void OnCancel() override;

    // kcp output
    static int KCPOutput(const char *buf, int len, struct IKCPCB *kcp, void *user);

    KCPAPI api_;
    std::shared_ptr<UDPInterface> udp_;
    IP4Address peer_;
    std::vector<char> recv_buf_;
    KCPStreamCallback *cb_ = nullptr;
    uint32_t conv_ = 0;
    bool closed_ = true;
};

class KCPStreamAdapter : public KCPStreamInterface {
public:
    static std::unique_ptr<KCPStreamAdapter> Create(
        std::shared_ptr<UDPInterface> udp, const IP4Address& peer, uint32_t conv);

    explicit KCPStreamAdapter(std::unique_ptr<KCPStreamInterface> impl)
        : impl_(std::move(impl)) {}

    ~KCPStreamAdapter() {
        executor()->Invoke([this] {
            impl_->Close();
            impl_.reset();
        });
    }

    bool Open(const KCPConfig& config, KCPStreamCallback *cb) override {
        return executor()->Invoke(&KCPStreamInterface::Open, impl_.get(), config, cb);
    }

    void Close() override {
        return executor()->Invoke(&KCPStreamInterface::Close, impl_.get());
    }

    bool Write(const char *buf, size_t len) override {
        return executor()->Invoke(&KCPStreamInterface::Write, impl_.get(), buf, len);
    }

    const IP4Address& local_address() const override {
        return impl_->local_address();
    }

    const IP4Address& remote_address() const override {
        return impl_->remote_address();
    }

    uint32_t conv() const override {
        return impl_->conv();
    }

    ExecutorInterface *executor() override {
        return impl_->executor();
    }
private:
    std::unique_ptr<KCPStreamInterface> impl_;
};
}

#endif // ! _KCP_STREAM_H_INCLUDED
