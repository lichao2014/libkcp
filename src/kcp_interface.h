#ifndef _KCP_INTERFACE_H_INCLUDED
#define _KCP_INTERFACE_H_INCLUDED

#include <system_error>

#include "common_types.h"

namespace kcp {
class KCPStreamCallback {
protected:
    virtual ~KCPStreamCallback() = default;
public:
    virtual void OnRecvKCP(const char *buf, size_t size) = 0;
    virtual bool OnError(const std::error_code& ec) = 0;
};

class KCPStreamInterface {
public:
    virtual ~KCPStreamInterface() = default;

    virtual bool Open(const KCPConfig& config, KCPStreamCallback *cb) = 0;
    virtual void Close() = 0;
    virtual bool Write(const char *buf, size_t len) = 0;
    virtual const IP4Address& local_address() const = 0;
    virtual const IP4Address& remote_address() const = 0;
    virtual uint32_t conv() const = 0;
    virtual ExecutorInterface *executor() = 0;
};

class KCPClientCallback {
protected:
    virtual ~KCPClientCallback() = default;
public:
    virtual void OnConnected(bool success) = 0;
    virtual void OnClose() = 0;
    virtual void OnRecvKCP(const char *buf, size_t size) = 0;
    virtual bool OnError(const std::error_code& ec) = 0;
};

class KCPClientInterface {
public:
    virtual ~KCPClientInterface() = default;

    virtual bool Open(KCPClientCallback *cb) = 0;
    virtual bool Write(const char *buf, size_t len) = 0;
    virtual void Close() = 0;
    virtual uint32_t conv() const = 0;
    virtual const IP4Address& local_address() const = 0;
    virtual const IP4Address& remote_address() const = 0;
    virtual ExecutorInterface *executor() = 0;
};

class KCPServerCallback {
protected:
    virtual ~KCPServerCallback() = default;
public:
    virtual bool OnAccept(std::unique_ptr<KCPClientInterface> client) = 0;
    virtual bool OnError(const std::error_code& ec) = 0;
};

class KCPServerInterface {
public:
    virtual ~KCPServerInterface() = default;

    virtual bool Start(KCPServerCallback *cb) = 0;
    virtual void Stop() = 0;
    virtual const IP4Address& local_address() const = 0;
    virtual ExecutorInterface *executor() = 0;
};

constexpr size_t kDefaultRecvSize = 1024 * 64;

struct KCPServerConfig : KCPConfig {
    size_t recv_size = kDefaultRecvSize;
};

class KCPContextInterface {
public:
    static std::unique_ptr<KCPContextInterface> Create(size_t thread_num = 0);

    virtual ~KCPContextInterface() = default;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual ExecutorInterface *executor() = 0;
    virtual std::unique_ptr<KCPStreamInterface> CreateStream(
        const IP4Address& addr, const IP4Address& peer, uint32_t conv) = 0;
    virtual std::unique_ptr<KCPClientInterface> CreateClient(
        const IP4Address& addr, const IP4Address& peer, const KCPConfig& config) = 0;
    virtual std::unique_ptr<KCPServerInterface> CreateServer(
        const IP4Address& addr, const KCPServerConfig& config) = 0;
};
}

#endif // !_KCP_INTERFACE_H_INCLUDED
