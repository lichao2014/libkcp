#include "asio_udp.h"

#include <iostream>

using namespace kcp;

namespace {
constexpr uint32_t kWaitForPrecision = 4;

bool Address2Endpoint(const IP4Address& from,
                      boost::asio::ip::udp::endpoint *to) {
    KCP_ASSERT(to);

    try {
        to->address(boost::asio::ip::address_v4(htonl(from.ip4)));
        to->port(from.port);
    } catch (...) {
        return false;
    }

    return true;
}

void Endpoint2Address(const boost::asio::ip::udp::endpoint& from,
                      IP4Address *to) noexcept {
    KCP_ASSERT(to);

    to->ip4 = ntohl(from.address().to_v4().to_uint());
    to->port = from.port();
    to->conv = 0;
}

class BoostErrorCategory : public std::error_category {
public:
    explicit BoostErrorCategory(const boost::system::error_category& category)
        : category_(category) {}

    const char *name() const noexcept {
        return category_.name();
    }

    std::string message(int err) const {
        return category_.message(err);
    }

private:
    const boost::system::error_category& category_;
};
}

//static
std::shared_ptr<IOContextInterface>
IOContextInterface::Create(size_t thread_num) {
    return AsioIOContext::Create(thread_num);
}

void IOContextThread::Start() {
    thread_ = std::thread([this] {
        StartTimer();
        run();
        CancelAllTasks();
    });
}

void IOContextThread::Stop() {
    if (stopped()) {
        return;
    }

    StopTimer();
    stop();

    if (thread_.joinable()) {
        thread_.join();
    }
}

bool IOContextThread::CanNowExecuted() const {
    return stopped() || thread_.get_id() == std::this_thread::get_id();
}

bool IOContextThread::DispatchTask(void *key, TaskInterface *task) {
    boost::asio::dispatch(*this, [key, task, this]{
        uint32_t now = Now32();
        uint32_t delay = task->OnRun(now);
        if (TaskInterface::kNotContinue != delay) {
            tasks_.emplace(now + delay, std::make_pair(key, task));
        }
    });

    return true;
}

void IOContextThread::CancelTask(void *key) {
    boost::asio::dispatch(
        *this,

        boost::asio::use_future([key, this] {
            auto it = tasks_.begin();
            while (tasks_.end() != it) {
                if (it->second.first == key) {
                    it->second.second->OnCancel();
                    it = tasks_.erase(it);
                } else {
                    ++it;
                }
            }
        })
    ).wait();
}

uint32_t IOContextThread::RunTasks() {
    uint32_t now = Now32();

    while (!tasks_.empty()) {
        auto task_pair = tasks_.begin()->second;

        int32_t diff = TimeDiff(tasks_.begin()->first, now);
        if (diff > 0) {
            return diff;
        }

        tasks_.erase(tasks_.begin());

        uint32_t delay = task_pair.second->OnRun(now);
        if (TaskInterface::kNotContinue != delay) {
            tasks_.emplace(now + delay, std::move(task_pair));
        }
    }

    return 0;
}

void IOContextThread::CancelAllTasks() {
    for (auto && task : tasks_) {
        task.second.second->OnCancel();
    }

    tasks_.clear();
}

void IOContextThread::StartTimer() {
    uint32_t delay = RunTasks();
    if (delay <= kWaitForPrecision) {
        boost::asio::post(*this, [this] { StartTimer(); });
        return;
    }

    timer_.expires_from_now(std::chrono::milliseconds(delay));

    timer_.async_wait([this](const boost::system::error_code& ec) {
        if (ec) {
            return;
        }

        StartTimer();
    });
}

void IOContextThread::StopTimer() {
    timer_.cancel();
}

AsioUDP::AsioUDP(std::shared_ptr<IOContextThread> io_ctx)
    : socket_(*io_ctx, boost::asio::ip::udp::v4())
    , recv_buf_()
    , io_ctx_(io_ctx) {
}

bool AsioUDP::Bind(const IP4Address& addr) {
    boost::asio::ip::udp::endpoint endpoint;
    if (!Address2Endpoint(addr, &endpoint)) {
        return false;
    }

    boost::system::error_code ec;
    socket_.bind(endpoint, ec);
    if (ec) {
        return false;
    }

    address_ = addr;
    return true;
}

bool AsioUDP::Open(UDPCallback *cb) {
    if (0 == recv_buf_size_) {
        return false;
    }

    cb_ = cb;
    closed_ = false;

    StartRead();
    return true;
}

void AsioUDP::Close() {
    if (socket_.is_open()) {
        socket_.shutdown(boost::asio::socket_base::shutdown_send);
        socket_.close();
    }

    if (closed_) {
        return;
    }

    closed_ = true;
    cb_ = nullptr;

    io_ctx_->CancelTask(this);
}

void AsioUDP::SetRecvBufSize(size_t recv_size) {
    if (recv_size > recv_buf_size_) {
        recv_buf_size_ = recv_size;
    }
}

bool AsioUDP::Send(const IP4Address& to, const char *buf, size_t len) {
    if (closed_) {
        return false;
    }

    write_bufs_.PutBuf(to, Buffer::New(len, buf));
    TryStartWrite();

    return true;
}

const IP4Address& AsioUDP::local_address() const {
    return address_;
}

ExecutorInterface *AsioUDP::executor() {
    return io_ctx_.get();
}

void AsioUDP::TryStartWrite() {
    if (in_writing_) {
        return;
    }

    if (in_writing_buf_.Empty()) {
        IP4Address peer;
        if (!write_bufs_.GetBufs(&peer, &in_writing_buf_)) {
            return;
        }

        if (!Address2Endpoint(peer, &in_writing_peer_)) {
            // TODO:
            // error callback
            return;
        }
    }

    if (in_writing_buf_.Empty()) {
        return;
    }

    socket_.async_send_to(
        in_writing_buf_,
        in_writing_peer_,

        [sp = shared_from_this()]
        (const boost::system::error_code& ec, std::size_t bytes_transferred) mutable {
            if (ec && sp->ErrorCallback(ec)) {
                return;
            }

            sp->WriteCallback(bytes_transferred);
        }
    );

    in_writing_ = true;
}

void AsioUDP::StartRead() {
    // TODO :
    // fix bug : changing recv buf size will lost data
    if (recv_buf_.size() < recv_buf_size_) {
        recv_buf_.resize(recv_buf_size_);
    }

    socket_.async_receive_from(
        boost::asio::buffer(recv_buf_),
        peer_,

        [sp = shared_from_this()]
        (const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (ec && sp->ErrorCallback(ec)) {
                return;
            }

            if (bytes_transferred > 0) {
                sp->ReadCallback(bytes_transferred);
            }
        }
    );
}

void AsioUDP::WriteCallback(std::size_t bytes_transferred) {
    in_writing_buf_.Cut(bytes_transferred);
    in_writing_ = false;
    TryStartWrite();
}

void AsioUDP::ReadCallback(std::size_t bytes_transferred) {
    if (cb_) {
        IP4Address from;
        Endpoint2Address(peer_, &from);

        cb_->OnRecvUDP(from, recv_buf_.data(), bytes_transferred);
    }

    StartRead();
}

bool AsioUDP::ErrorCallback(const boost::system::error_code& ec) {
    if (closed_ || !cb_) {
        return true;
    }

    return cb_->OnError({ ec.value(), BoostErrorCategory(ec.category()) });
}

// static
std::shared_ptr<AsioIOContext>
AsioIOContext::Create(size_t thread_num) {
    return { new AsioIOContext(thread_num), [](auto *p) { delete p; } };
}

AsioIOContext::AsioIOContext(size_t thread_num) {
    if (0 == thread_num) {
        thread_num = std::thread::hardware_concurrency();
    }

    for (size_t i = 0; i < thread_num; ++i) {
        threads_.push_back(std::make_shared<IOContextThread>());
    }
}

void AsioIOContext::Start() {
    for (auto&& thread : threads_) {
        thread->Start();
    }
}

void AsioIOContext::Stop() {
    for (auto&& thread : threads_) {
        thread->Stop();
    }
}

std::shared_ptr<UDPInterface>
AsioIOContext::CreateUDP(const IP4Address& addr) {
    std::shared_ptr<AsioUDP> udp{
        new AsioUDP(SelectIOThread()),
        [](AsioUDP *p) { delete p; }
    };

    if (!udp->Bind(addr)) {
        return nullptr;
    }

    return udp;
}

ExecutorInterface *AsioIOContext::executor() {
    return SelectIOThread().get();
}

const std::shared_ptr<IOContextThread>&
AsioIOContext::SelectIOThread() {
    return threads_[select_thread_index_++ % threads_.size()];
}
