#ifndef _KCP_STREAM_H_INCLUDED
#define _KCP_STREAM_H_INCLUDED

#include <memory>
#include <cstdint>
#include <list>

#include "ikcp.h"

namespace kcp {
class KCPStream : public std::shared_ptr<ikcpcb> {
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
}

#endif // ! _KCP_STREAM_H_INCLUDED
