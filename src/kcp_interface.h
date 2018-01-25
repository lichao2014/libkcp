#ifndef _KCP_INTERFACE_H_INCLUDED
#define _KCP_INTERFACE_H_INCLUDED

#include "common_types.h"

namespace kcp {
struct KCPConfig {
    int mtu = kKCPMTUDefault;
    int interval = 10;
    int resend = 2;
    int min_rto = 10;
    int sndwnd = 32;
    int rcvwnd = 32;
    bool nodelay = true;
    bool nocwnd = false;
};

class KCPClientCallback {
protected:
    virtual ~KCPClientCallback() = default;
public:
    virtual void OnRecvKCP(const char *buf, size_t size) = 0;
    virtual void OnError(int err, const char *what) = 0;
};

class KCPClientInterface {
protected:
    virtual ~KCPClientInterface() = default;
public:
    virtual bool Connect(const UDPAddress& to, uint32_t conv, const KCPConfig& config, KCPClientCallback *cb) = 0;
    virtual bool Write(const char *buf, std::size_t len) = 0;
    virtual void Close() = 0;
    virtual const UDPAddress& local_address() const = 0;
    virtual const UDPAddress& remote_address() const = 0;
    virtual uint32_t conv() const = 0;
    virtual ExecutorInterface *executor() = 0;
};

class KCPServerCallback {
protected:
    virtual ~KCPServerCallback() = default;
public:
    virtual bool OnAccept(std::shared_ptr<KCPClientInterface> client, const UDPAddress& from, uint32_t conv) = 0;
};

class KCPServerInterface {
protected:
    virtual ~KCPServerInterface() = default;
public:
    virtual bool Start(KCPServerCallback *cb) = 0;
    virtual void Stop() = 0;
    virtual const UDPAddress& local_address() const = 0;
    virtual ExecutorInterface *executor() = 0;
};

class KCPContextInterface {
public:
    static std::unique_ptr<KCPContextInterface> Create();

    virtual ~KCPContextInterface() = default;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual ExecutorInterface *executor() = 0;
    virtual std::shared_ptr<KCPClientInterface> CreateClient(const UDPAddress& addr) = 0;
    virtual std::shared_ptr<KCPServerInterface> CreateServer(const UDPAddress& addr) = 0;
};
}

#endif // !_KCP_INTERFACE_H_INCLUDED