#include "kcp_stream.h"
#include "logging.h"

using namespace kcp;

//static
bool KCPAPI::ParseConv(const char *buf, size_t len, uint32_t *conv) {
    // conv is le32
    if (len < 4) {
        return false;
    }

    if (conv) {
        *conv = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    }

    return true;
}

bool KCPStream::Open(const KCPConfig& config, KCPStreamCallback *cb) {
    if (!api_.Init(conv_, &KCPStream::KCPOutput, this)) {
        return false;
    }

    api_.set_mtu(config.mtu);
    api_.set_wndsize(config.sndwnd, config.rcvwnd);
    api_.set_nodelay(config.nodelay ? 1 : 0,
                     config.interval,
                     config.resend,
                     config.nocwnd ? 1 : 0);
    api_->rx_minrto = config.min_rto;

    cb_ = cb;
    closed_ = false;

    KCP_ASSERT(udp_);
    if (!udp_->executor()->DispatchTask(this, this)) {
        return false;
    }

    udp_->SetRecvBufSize(config.mtu);
    return udp_->Open(this);
}

void KCPStream::Close() {
    if (closed_) {
        return;
    }

    cb_ = nullptr;
    closed_ = true;

    if (udp_) {
        udp_->executor()->CancelTask(this);
        udp_->Close();
    }
}

bool KCPStream::Write(const char *buf, size_t len) {
    if (closed_) {
        return false;
    }

    KCP_LOG(kInfo) << "kcp send len=" << len << std::endl;
    return api_.Send(buf, len);
}

const IP4Address& KCPStream::local_address() const {
    return udp_->local_address();
}

const IP4Address& KCPStream::remote_address() const {
    return peer_;
}

uint32_t KCPStream::conv() const {
    return api_->conv;
}

ExecutorInterface *KCPStream::executor() {
    return udp_->executor();
}

void KCPStream::WriteUDP(const char *buf, std::size_t len) {
    KCP_LOG(kInfo) << "udp send len=" << len << std::endl;

    if (!udp_->Send(peer_, buf, len)) {
        OnKCPError(ErrNum::kUDPSendFailed);
    }
}

void KCPStream::TryRecvKCP() {
    int size = 0;
    while ((size = api_.peek_size()) > 0) {
        KCP_LOG(kInfo) << "kcp recv len=" << size << std::endl;

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

bool KCPStream::OnRecvUDP(const IP4Address& from, const char *buf, size_t len) {
    if (!(from == peer_)) {
        OnKCPError(ErrNum::kBadIP4Address);;
        return false;
    }

    KCP_LOG(kInfo) << "udp recv len=" << len
        << ",from=" << from.ip4_string() << "," << from.port << std::endl;

    if (!api_.Input(buf, len)) {
        OnKCPError(ErrNum::kKCPInputFailed);
        return false;
    }

    TryRecvKCP();

    return true;
}

bool KCPStream::OnError(const std::error_code& ec) {
    if (!cb_) {
        return true;
    }

    return cb_->OnError(ec);
}

uint32_t KCPStream::OnRun(uint32_t now) {
    if (closed_) {
        return kNotContinue;
    }

    int32_t delay = TimeDiff(api_.Check(now), now);
    if (delay <= 0) {
        api_.Update(now);
        delay = 1;
    }

    return delay;
}

void KCPStream::OnCancel() {}

// static
int KCPStream::KCPOutput(const char *buf, int len, struct IKCPCB *kcp, void *user) {
    KCP_LOG(kInfo) << "kcp output conv=" << kcp->conv
        << ",len=" << len << std::endl;

    KCPStream *stream = reinterpret_cast<KCPStream *>(user);
    stream->WriteUDP(buf, len);
    return 0;
}

// static
std::unique_ptr<KCPStreamAdapter>
KCPStreamAdapter::Create(std::shared_ptr<UDPInterface> udp,
                         const IP4Address& peer,
                         uint32_t conv) {
    auto stream = std::make_unique<KCPStream>(udp, peer, conv);
    if (!stream) {
        return nullptr;
    }

    return std::make_unique<KCPStreamAdapter>(std::move(stream));
}
