#ifndef _UDP_INTERFACE_H_INCLUDED
#define _UDP_INTERFACE_H_INCLUDED

#include <system_error>

#include "common_types.h"

namespace kcp {
class UDPCallback {
protected:
    virtual ~UDPCallback() = default;
public:
    virtual bool OnRecvUDP(const IP4Address& from, const char *buf, size_t len) = 0;
    virtual bool OnError(const std::error_code& ec) = 0;
};

class UDPInterface {
protected:
    virtual ~UDPInterface() = default;
public:
    virtual bool Open(size_t recv_size, UDPCallback *cb) = 0;
    virtual void Close() = 0;
    virtual bool Send(const IP4Address& to, const char *buf, size_t len) = 0;
    virtual const IP4Address& local_address() const = 0;
    virtual ExecutorInterface *executor() = 0;
};

class IOContextInterface {
protected:
    virtual ~IOContextInterface() = default;
public:
    static std::shared_ptr<IOContextInterface> Create(size_t thread_num);

    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual std::shared_ptr<UDPInterface> CreateUDP(const IP4Address& addr) = 0;
    virtual ExecutorInterface *executor() = 0;
};
}

#endif // !_UDP_INTERFACE_H_INCLUDED
