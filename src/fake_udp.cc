#include "fake_udp.h"
#include <chrono>

namespace kcp {
uint32_t Now32() {
    using namespace std::chrono;
    auto now64 = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
    return static_cast<uint32_t>(now64);
}
}

using namespace kcp;

bool FakeUDP::Open(UDPCallback *cb) {
    cb_ = cb;
    return true;
}

void FakeUDP::Close() {
    cb_ = nullptr;
}

bool FakeUDP::Send(const UDPAddress& to, const char *buf, size_t len) {
    return ctx_->Send(this, to, buf, len);
}

const UDPAddress& FakeUDP::local_address() const {
    return address_;
}

ExecutorInterface *FakeUDP::executor() {
    return ctx_->executor();
}

//static 
std::shared_ptr<FakeIOContext> FakeIOContext::Create() {
    return { new FakeIOContext, [](FakeIOContext *p) { delete p; } };
}

void FakeIOContext::Start() {
    closed_ = false;
    thread_ = std::thread([this] { RunTasks(); });
}

void FakeIOContext::Stop() {
    closed_ = true;
    tasks_cond_.notify_all();

    if (thread_.joinable()) {
        thread_.join();
    }
}

std::shared_ptr<UDPInterface> 
FakeIOContext::CreateUDP(const UDPAddress& addr) {
    std::unique_lock<std::mutex> guard(udps_mu_);

    if (udps_.end() != udps_.find(addr)) {
        return nullptr;
    }

    std::shared_ptr<FakeUDP> udp {
        new FakeUDP(this, addr),

        [sp = shared_from_this()](FakeUDP *p) {
            std::unique_lock<std::mutex> guard(sp->udps_mu_);
            sp->udps_.erase(p->local_address());
            delete p;
        }
    };

    udps_.emplace(addr, udp.get());

    return udp;
}

bool FakeIOContext::Send(FakeUDP *from, const UDPAddress& to, const char *buf, size_t len) {
    if (closed_) {
        return false;
    }

    std::unique_lock<std::mutex> guard(udps_mu_);

    auto it = udps_.find(to);
    if (udps_.end() == it) {
        return false;
    }

    if (!it->second->cb_) {
        return false;
    }

    it->second->cb_->OnRecvUdp(from->local_address(), buf, len);

    return true;
}

ExecutorInterface *FakeIOContext::executor() {
    return this;
}

bool FakeIOContext::IsCurrentThread() const {
    return closed_ || std::this_thread::get_id() == thread_.get_id();
}

bool FakeIOContext::DispatchTask(std::shared_ptr<TaskInterface> task) {
    if (closed_) {
        return false;
    }

    std::unique_lock<std::recursive_mutex> guard(tasks_mu_);
    tasks_.emplace(Now32(), task);
    tasks_cond_.notify_one();
    return true;
}

void FakeIOContext::RunTasks() {
    uint32_t now;

    while (!closed_) {
        std::unique_lock<std::recursive_mutex> guard(tasks_mu_);

        if (tasks_.empty()) {
            tasks_cond_.wait(
                guard, 
                [&] { return closed_ || !tasks_.empty(); }
            );
        } else {
            now = Now32();

            if (tasks_.begin()->first > now) {
                uint32_t ms = tasks_.begin()->first - now;
                if (ms > 20) {
                    tasks_cond_.wait_for(
                        guard,
                        std::chrono::milliseconds(ms),
                        [&] { return closed_ || !tasks_.empty(); }
                    );
                }
            }
        }

        if (closed_) {
            break;
        }

        now = Now32();

        while (!tasks_.empty()) {
            auto task = tasks_.begin()->second;
            if (now < tasks_.begin()->first) {
                break;
            }

            tasks_.erase(tasks_.begin());

            uint32_t delay = task->OnRun(now, false);
            if (TaskInterface::kNotContinue != delay) {
                tasks_.emplace(now + delay, task);
            }
        }
    }

    ClearTasks();
}

void FakeIOContext::ClearTasks() {
    std::unique_lock<std::recursive_mutex> guard(tasks_mu_);

    for (auto && task : tasks_) {
        task.second->OnRun(0, true);
    }

    tasks_.clear();
}
