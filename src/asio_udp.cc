#include "asio_udp.h"

using namespace kcp;

namespace {
bool Address2Endpoint(const UDPAddress& from, boost::asio::ip::udp::endpoint& to) {
    try {
        to.address(boost::asio::ip::address_v4(htonl(from.ip4)));
        to.port(from.port);
    } catch (...) {
        return false;
    }

    return true;
}

void Endpoint2Address(const boost::asio::ip::udp::endpoint& from, UDPAddress& to) {
    to.ip4 = ntohl(from.address().to_v4().to_uint());
    to.port = from.port();
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
        while (!stopped()) {
            uint32_t delay = RunTasks();
            if (0 == delay) {
                run_one();
            } else if (delay < 8) {
                poll();
            } else {
                run_for(std::chrono::milliseconds(delay));
            }
        }

        ClearTasks();
    });
}

void IOContextThread::Stop() {
    if (stopped()) {
        return;
    }

    stop();

    if (thread_.joinable()) {
        thread_.join();
    }
}

bool IOContextThread::IsCurrentThread() const {
    return stopped() || thread_.get_id() == std::this_thread::get_id();
}

bool IOContextThread::DispatchTask(void *key, 
                                   std::shared_ptr<TaskInterface> task) {
    boost::asio::dispatch(*this, [key, task, this]{
        uint32_t now = Now32();
        uint32_t delay = task->OnRun(now, false);
        if (TaskInterface::kNotContinue != delay) {
            tasks_.emplace(now + delay, std::make_pair(key, task));
        }
    });

    return true;
}

void IOContextThread::CancelTask(void *key) {
    boost::asio::dispatch(*this, [key, this] {
        auto it = tasks_.begin();
        while (tasks_.end() != it) {
            if (it->second.first == key) {
                it->second.second->OnRun(0, true);
                it = tasks_.erase(it);
            } else {
                ++it;
            }
        }
    });
}

uint32_t IOContextThread::RunTasks() {
    uint32_t now = Now32();

    while (!tasks_.empty()) {
        auto task = tasks_.begin()->second;
        if (now < tasks_.begin()->first) {
            return tasks_.begin()->first - now;
        }

        tasks_.erase(tasks_.begin());

        uint32_t delay = task.second->OnRun(now, false);
        if (TaskInterface::kNotContinue != delay) {
            tasks_.emplace(now + delay, task);
        }
    }

    return 0;
}

void IOContextThread::ClearTasks() {
    for (auto && task : tasks_) {
        task.second.second->OnRun(0, true);
    }

    tasks_.clear();
}

bool AsioUDP::Open(size_t recv_size, UDPCallback *cb) {
    recv_buf_.resize(recv_size);
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

bool AsioUDP::Send(const UDPAddress& to, const char *buf, size_t len) {
    boost::asio::ip::udp::endpoint peer;
    if (!Address2Endpoint(to, peer)) {
        return false;
    }

    Write(WriteReq::New(peer, buf, len));

    return true;
}

const UDPAddress& AsioUDP::local_address() const {
    return address_;
}

ExecutorInterface *AsioUDP::executor() {
    return io_ctx_.get();
}

AsioUDP::AsioUDP(std::shared_ptr<IOContextThread> io_ctx)
    : socket_(*io_ctx, boost::asio::ip::udp::v4())
    , recv_buf_()
    , io_ctx_(io_ctx) {
}

bool AsioUDP::Init(const UDPAddress& addr) {
    boost::asio::ip::udp::endpoint ep;
    if (!Address2Endpoint(addr, ep)) {
        return false;
    }

    boost::system::error_code ec;
    socket_.bind(ep, ec);
    if (ec) {
        return false;
    }

    address_ = addr;
    return true;
}

void AsioUDP::Write(WriteReq::Ptr req) {
    write_req_queue_.push(std::move(req));

    if (!in_writing_) {
        StartWrite();
    }
}

void AsioUDP::StartWrite() {
    if (!write_req_queue_.empty()) {
        WriteReq::Ptr req = std::move(write_req_queue_.front());
        write_req_queue_.pop();

        WriteReq *req_ptr = req.get();
        socket_.async_send_to(
            boost::asio::buffer(req_ptr->buf(), req_ptr->len()),
            req_ptr->peer,

            [sp = shared_from_this(), req = std::move(req)]
            (const boost::system::error_code& ec, std::size_t bytes_transferred) mutable {
                if (ec && sp->ErrorCallback(ec)) {
                    return;
                }

                sp->WriteCallback(std::move(req), bytes_transferred);
            }
        );

        in_writing_ = true;
    }
}

void AsioUDP::StartRead() {
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

void AsioUDP::WriteCallback(WriteReq::Ptr req, std::size_t bytes_transferred) {
    if (req->len() > bytes_transferred) {
        req->offset += bytes_transferred;
        write_req_queue_.push(std::move(req));
    }

    in_writing_ = false;
    StartWrite();
}

void AsioUDP::ReadCallback(std::size_t bytes_transferred) {
    UDPAddress from;
    Endpoint2Address(peer_, from);

    cb_->OnRecvUDP(from, recv_buf_.data(), bytes_transferred);
    StartRead();
}

bool AsioUDP::ErrorCallback(const boost::system::error_code& ec) {
    if (closed_) {
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
AsioIOContext::CreateUDP(const UDPAddress& addr) {
    auto index = select_thread_index_++;

    std::shared_ptr<AsioUDP> udp{
        new AsioUDP(threads_[index % threads_.size()]),
        [](AsioUDP *p) { delete p; }
    };

    if (!udp->Init(addr)) {
        return nullptr;
    }

    return udp;
}