#ifndef _FAKE_UDP_H_INCLUDED
#define _FAKE_UDP_H_INCLUDED

#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "udp_interface.h"

namespace kcp {
class FakeUDP : public UDPInterface {
public:
    void Start(UDPCallback *cb) override;
    void Stop() override;
    bool Send(const UDPAddress& to, const char *buf, size_t len) override;
    const UDPAddress& local_address() const override;
    ExecutorInterface *executor() override;
private:
    friend class FakeIOContext;

    FakeUDP(FakeIOContext * ctx, const UDPAddress& address)
        : ctx_(ctx)
        , address_(address) {}

    UDPAddress address_;
    FakeIOContext *ctx_ = nullptr;
    UDPCallback *cb_ = nullptr;
};

class FakeIOContext : public ExecutorInterface
                    , public IOContextInterface
                    , public std::enable_shared_from_this<FakeIOContext> {
public:
    static std::shared_ptr<FakeIOContext> Create();

    void Start() override;
    void Stop() override;
    std::shared_ptr<UDPInterface> CreateUDP(const UDPAddress& addr) override;
    ExecutorInterface *executor() override;
private:
    FakeIOContext() = default;
    ~FakeIOContext() = default;

    friend class FakeUDP;
    // executor interface
    bool IsCurrentThread() const override;
    bool DispatchTask(std::shared_ptr<TaskInterface> task) override;

    bool Send(FakeUDP *from, const UDPAddress& to, const char *buf, size_t len);
    // only for test
    void RunTasks();
    void ClearTasks();

    std::mutex udps_mu_;
    std::map<UDPAddress, FakeUDP *> udps_;

    std::recursive_mutex tasks_mu_;
    std::condition_variable_any tasks_cond_;
    std::multimap<uint32_t, std::shared_ptr<TaskInterface>> tasks_;

    std::thread thread_;
    std::atomic_bool closed_ = false;
};
}

#endif // !_FAKE_UDP_H_INCLUDED
