#ifndef _UDP_INTERFACE_H_INCLUDED
#define _UDP_INTERFACE_H_INCLUDED

#include "common_types.h"

namespace kcp {
class UDPCallback {
protected:
    virtual ~UDPCallback() = default;
public:
    virtual void OnRecvUdp(const UDPAddress& from, const char *buf, size_t len) = 0;
    virtual bool OnError(int err, const char *what) = 0;
};

class UDPInterface {
protected:
    virtual ~UDPInterface() = default;
public:
    virtual void Start(UDPCallback *cb) = 0;
    virtual void Stop() = 0;
    virtual bool Send(const UDPAddress& to, const char *buf, size_t len) = 0;
    virtual const UDPAddress& local_address() const = 0;
    virtual ExecutorInterface *executor() = 0;
};

class IOContextInterface {
protected:
    virtual ~IOContextInterface() = default;
public:
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual std::shared_ptr<UDPInterface> CreateUDP(const UDPAddress& addr) = 0;
    virtual ExecutorInterface *executor() = 0;

    static std::shared_ptr<IOContextInterface> Create(size_t thread_num);
};
}

#endif // !_UDP_INTERFACE_H_INCLUDED