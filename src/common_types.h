#ifndef _COMMON_TYPES_H_INCLUDED
#define _COMMON_TYPES_H_INCLUDED

#include <cstdint>
#include <string>
#include <limits>
#include <memory>
#include <tuple>
#include <future>

namespace kcp {
constexpr size_t kEthMTU = 1500;
constexpr size_t kIPHeaderSize = 20;
constexpr size_t kUDPHeadSize = 8;
constexpr size_t kKCPHeadSize = 24;
constexpr size_t kKCPMTUDefault = kEthMTU - kIPHeaderSize - kUDPHeadSize - kKCPHeadSize;

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

union UDPAddress {
    uint64_t u64;

    struct {
        uint32_t ip4;
        uint16_t port;
        uint16_t conv;
    };

    constexpr UDPAddress() : u64(0) {}

    constexpr UDPAddress(const UDPAddress& addr, uint16_t u16)
        : u64(addr.u64)
        , conv(u16) {}

    constexpr bool operator <(const UDPAddress& rhs) const {
        return u64 < rhs.u64;
    }

    constexpr bool operator ==(const UDPAddress& rhs) const {
        return u64 == rhs.u64;
    }

    UDPAddress(const char *ip4, uint16_t port, uint16_t conv = 0);
    std::string ip4_string() const;
};

uint32_t Now32();

class TaskInterface {
protected:
    virtual ~TaskInterface() = default;
public:
    static constexpr uint32_t kNotContinue = (std::numeric_limits<uint32_t>::max)();
    virtual uint32_t OnRun(uint32_t now, bool cancel) = 0;
};

template<typename Sig>
class FunctionTask;

template<typename Fn, typename ... Args>
class FunctionTask<Fn(Args...)> : public TaskInterface {
public:
    template<typename T, typename ... Ts>
    FunctionTask(T&& fn, Ts&& ... args)
        : fn_(std::forward<T>(fn))
        , args_(std::forward<Args>(args)...){}

private:
    uint32_t OnRun(uint32_t now, bool cancel) override {
        std::apply(fn_, args_);
        return kNotContinue;
    }

    Fn fn_;
    std::tuple<Args...> args_;
};

template<typename Fn, typename ... Args>
auto NewTask(Fn&& fn, Args&& ... args) {
     return std::make_shared<FunctionTask<Fn(Args...)>>(
         std::forward<Fn>(fn), std::forward<Args>(args)...);
}

class ExecutorInterface {
protected:
    virtual ~ExecutorInterface() = default;
public:
    virtual bool IsCurrentThread() const = 0;
    virtual bool DispatchTask(void *key, std::shared_ptr<TaskInterface> task) = 0;
    virtual void CancelTask(void *key) = 0;

    template<typename Fn, typename ... Args>
    void Post(Fn&& fn, Args&& ... args) {
        if (!DispatchTask(nullptr, 
                          NewTask(std::forward<Fn>(fn), 
                          std::forward<Args>(args)...))) {
            throw std::logic_error("executor disaptch failed");
        }
    }

    template<typename Fn, typename ... Args>
    auto Invoke(Fn&& fn, Args&& ... args) 
        -> std::enable_if_t<std::is_void_v<std::result_of_t <Fn(Args...)>>> {
        if (IsCurrentThread()) {
            std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);
            return;
        }

        std::promise<void> promise;
        Post([&] { 
            std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);
            promise.set_value();
        });

        return promise.get_future().get();
    }

    template<typename Fn, typename ... Args>
    auto Invoke(Fn&& fn, Args&& ... args)
        -> std::enable_if_t<!std::is_void_v<std::result_of_t <Fn(Args...)>>, 
            std::result_of_t <Fn(Args...)>> {
        if (IsCurrentThread()) {
            return std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);
        }

        std::promise<std::result_of_t <Fn(Args...)>> promise;
        Post([&] {
            promise.set_value(std::invoke(std::forward<Fn>(fn), 
                                          std::forward<Args>(args)...));
        });

        return promise.get_future().get();
    }
};
}

#endif // !_COMMON_TYPES_H_INCLUDED
