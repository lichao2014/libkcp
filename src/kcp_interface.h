#ifndef _KCP_INTERFACE_H_INCLUDED
#define _KCP_INTERFACE_H_INCLUDED

#include <system_error>

#include "common_types.h"

namespace kcp {
class KCPClientCallback {
protected:
    virtual ~KCPClientCallback() = default;
public:
    virtual void OnRecvKCP(const char *buf, size_t size) = 0;
    virtual bool OnError(const std::error_code& ec) = 0;
};

class KCPClientInterface {
protected:
    virtual ~KCPClientInterface() = default;
public:
    virtual bool Connect(const UDPAddress& to, 
                         uint32_t conv, 
                         const KCPConfig& config, 
                         KCPClientCallback *cb) = 0;
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
    virtual bool OnAccept(std::shared_ptr<KCPClientInterface> client, 
                          const UDPAddress& from, 
                          uint32_t conv) = 0;
    virtual bool OnError(const std::error_code& ec) = 0;
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
    static std::unique_ptr<KCPContextInterface> Create(size_t thread_num = 0);

    virtual ~KCPContextInterface() = default;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual ExecutorInterface *executor() = 0;
    virtual std::shared_ptr<KCPClientInterface> CreateClient(const UDPAddress& addr) = 0;
    virtual std::shared_ptr<KCPServerInterface> CreateServer(const UDPAddress& addr) = 0;
};
}

#endif // !_KCP_INTERFACE_H_INCLUDED
