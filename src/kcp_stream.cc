#include "kcp_stream.h"

using namespace kcp;

//static 
std::shared_ptr<KCPStream>
KCPStream::Create(std::shared_ptr<UDPInterface> udp,
                  const UDPAddress& peer,
                  uint32_t conv,
                  const KCPConfig& config) {
    return { 
        new KCPStream(udp, peer, conv, config), 
        [](KCPStream *p) { delete p; }
    };
}

KCPStream::~KCPStream() {
    Close();
}

bool KCPStream::Open(KCPClientCallback *cb) {
    if (!api_.Init(conv_, this)) {
        return false;
    }

    api_->output = &KCPStream::KCPOutput;
    api_.set_mtu(config_.mtu);
    api_.set_wndsize(config_.sndwnd, config_.rcvwnd);
    api_.set_nodelay(config_.nodelay ? 1 : 0,
                    config_.interval,
                    config_.resend,
                    config_.nocwnd ? 1 : 0);
    cb_ = cb;
    closed_ = false;

    assert(udp_);
    if (!udp_->executor()->DispatchTask(this, shared_from_this())) {
        return false;
    }

    return udp_->Open(config_.mtu, this);
}

bool KCPStream::Write(const char *buf, std::size_t len) {
    return api_.Send(buf, len);
}

void KCPStream::Close() {
    if (closed_) {
        return;
    }

    closed_ = true;

    if (udp_) {
        udp_->executor()->CancelTask(this);
        udp_->Close();
    }
}

void KCPStream::WriteUDP(const char *buf, std::size_t len) {
    if (!udp_->Send(peer_, buf, len)) {
        OnKCPError(ErrNum::kUDPSendFailed);
    }
}

void KCPStream::TryRecvKCP() {
    int size = 0;
    while ((size = api_.peek_size()) > 0) {
        if (recv_buf_.size() < size) {
            recv_buf_.resize(size);
        }

        api_.Recv(&recv_buf_[0], size);
        cb_->OnRecvKCP(&recv_buf_[0], size);
    }
}

bool KCPStream::OnKCPError(ErrNum err) {
    return OnError(MakeErrorCode(err));
}

void KCPStream::OnRecvUDP(const UDPAddress& from, const char *buf, size_t len) {
    if (!(from == peer_)) {
        OnKCPError(ErrNum::kBadUDPAddress);;
        return;
    }

    if (!api_.Input(buf, len)) {
        OnKCPError(ErrNum::kKCPInputFailed);
        return;
    }

    TryRecvKCP();
}

bool KCPStream::OnError(const std::error_code& ec) {
    if (!cb_) {
        return true;
    }

    return cb_->OnError(ec);
}

uint32_t KCPStream::OnRun(uint32_t now, bool cancel) {
    if (cancel || closed_) {
        return kNotContinue;
    }

    uint32_t ms = api_.Check(now);
    uint32_t delay = 1;

    if (ms <= now) {
        api_.Update(now);
    } else {
        delay = ms - now;
    }

    return delay;
}

// static 
int KCPStream::KCPOutput(const char *buf, int len, struct IKCPCB *kcp, void *user) {
    KCPStream *stream = reinterpret_cast<KCPStream *>(user);
    stream->WriteUDP(buf, len);
    return 0;
}
