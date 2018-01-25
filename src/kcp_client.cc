#include "kcp_client.h"

#include <iostream>

using namespace kcp;

//static 
std::shared_ptr<KCPClient> 
KCPClient::Create(std::shared_ptr<UDPInterface> udp) {
    return { new KCPClient(udp), [](KCPClient *p) { delete p; } };
}

KCPClient::~KCPClient() {
    Close();
}

bool KCPClient::Connect(const UDPAddress& to, uint32_t conv, const KCPConfig& config, KCPClientCallback *cb) {
    if (!stream_.Init(conv, this)) {
        return false;
    }

    stream_->output = &KCPClient::KCPOutput;
    stream_.set_mtu(config.mtu);
    stream_.set_wndsize(config.sndwnd, config.rcvwnd);
    stream_.set_nodelay(config.nodelay ? 1 : 0, config.interval, config.resend, config.nocwnd ? 1 : 0);

    peer_ = to;
    cb_ = cb;
    closed_ = false;

    if (!udp_->executor()->DispatchTask(shared_from_this())) {
        return false;
    }

    udp_->SetRecvBufSize(config.mtu);
    return udp_->Open(this);
}

bool KCPClient::Write(const char *buf, std::size_t len) {
    static int cnt = 0;
    std::clog << "kcp send:cnt=" << cnt++ << std::endl;
    return stream_.Send(buf, len);
}

void KCPClient::Close() {
    if (closed_) {
        return;
    }

    closed_ = true;

    if (udp_) {
        udp_->Close();
    }
}

const UDPAddress& KCPClient::local_address() const {
    return udp_->local_address();
}

const UDPAddress& KCPClient::remote_address() const {
    return peer_;
}

void KCPClient::WriteUDP(const char *buf, std::size_t len) {
    if (!udp_->Send(peer_, buf, len)) {
        cb_->OnError(1, "write udp failed");
    }
}

void KCPClient::TryRecvKCP() {
    int size = 0;
    while ((size = stream_.peek_size()) > 0) {
        recv_buf_.resize(size);
        stream_.Recv(&recv_buf_[0], size);
        cb_->OnRecvKCP(&recv_buf_[0], size);
    }
}

void KCPClient::OnRecvUdp(const UDPAddress& from, const char *buf, size_t len) {
    if (!(from == peer_)) {
        cb_->OnError(1, "from not equal to");
        return;
    }

    if (!stream_.Input(buf, len)) {
        cb_->OnError(1, "kcp input failed");
        return;
    }

    TryRecvKCP();
}

bool KCPClient::OnError(int err, const char *what) {
    cb_->OnError(err, what);
    return false;
}

uint32_t KCPClient::OnRun(uint32_t now, bool cancel) {
    if (cancel || closed_) {
        return kNotContinue;
    }

    uint32_t ms = stream_.Check(now);
    uint32_t delay = 1;

    if (ms <= now) {
        stream_.Update(now);
    } else {
        delay = ms - now;
    }

    return delay;
}

// static 
int KCPClient::KCPOutput(const char *buf, int len, struct IKCPCB *kcp, void *user) {
    KCPClient *impl = reinterpret_cast<KCPClient *>(user);
    impl->WriteUDP(buf, len);
    return 0;
}
