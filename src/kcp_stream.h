#ifndef _KCP_STREAM_H_INCLUDED
#define _KCP_STREAM_H_INCLUDED

#include <memory>
#include <cstdint>

#include "ikcp.h"

#include "udp_interface.h"
#include "kcp_interface.h"
#include "kcp_error.h"

namespace kcp {
class KCPAPI : public std::shared_ptr<ikcpcb> {
public:
    using std::shared_ptr<ikcpcb>::shared_ptr;

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

    bool Init(uint32_t conv, void *user) {
        auto kcp = ikcp_create(conv, user);
        if (kcp) {
            reset(kcp, &ikcp_release);
        }

        return kcp;
    }
};

class KCPStream : public UDPCallback
                , public TaskInterface
                , public std::enable_shared_from_this<KCPStream> {
public:
    static std::shared_ptr<KCPStream> Create(std::shared_ptr<UDPInterface> udp,
                                             const UDPAddress& peer,
                                             uint32_t conv,
                                             const KCPConfig& config);

    bool Open(KCPClientCallback *cb) ;
    bool Write(const char *buf, std::size_t len);
    void Close();
    const UDPAddress& peer() const { return peer_; }
    uint32_t conv() const { return api_->conv; }
private:
    explicit KCPStream(std::shared_ptr<UDPInterface> udp,
                       const UDPAddress& peer,
                       uint32_t conv,
                       const KCPConfig& config)
        : udp_(udp)
        , peer_(peer)
        , conv_(conv)
        , config_(config) {}

    ~KCPStream();

    void WriteUDP(const char *buf, std::size_t len);
    void TryRecvKCP();
    bool OnKCPError(ErrNum err);

    // udp callback
    void OnRecvUDP(const UDPAddress& from, const char *buf, size_t len) override;
    bool OnError(const std::error_code& ec) override;

    // task callback
    uint32_t OnRun(uint32_t now, bool cancel) override;

    // kcp output
    static int KCPOutput(const char *buf, int len, struct IKCPCB *kcp, void *user);

    KCPAPI api_;
    std::shared_ptr<UDPInterface> udp_;
    UDPAddress peer_;
    uint32_t conv_;
    KCPConfig config_;
    std::vector<char> recv_buf_;
    KCPClientCallback *cb_ = nullptr;
    bool closed_ = true;
};
}

#endif // ! _KCP_STREAM_H_INCLUDED
