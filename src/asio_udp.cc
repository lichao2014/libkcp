#include "asio_udp.h"

#include <map>
#include <iostream>

using namespace kcp;

namespace {
bool Address2Endpoint(const UDPAddress& from, boost::asio::ip::udp::endpoint& to) {
    try {
        to.address(boost::asio::ip::make_address_v4(from.ip4_string()));
        to.port(from.port);
    } catch (...) {
        return false;
    }

    return true;
}

bool Endpoint2Address(const boost::asio::ip::udp::endpoint& from, UDPAddress& to) {
    try {
        to.ip4 = from.address().to_v4().to_uint();
        to.port = from.port();
    } catch (...) {
        return false;
    }

    return true;
}
}

//static 
std::shared_ptr<IOContextInterface>
IOContextInterface::Create(size_t thread_num) {
    return std::make_shared<AsioIOContext>(thread_num);
}

void IOContextThread::Start() {
    thread_ = std::thread([this] {
        while (!stopped()) {
            uint32_t delay = RunDelayTasks();
            if (0 == delay) {
                run_one();
            } else if (delay < 10) {
                poll();
            } else {
                run_for(std::chrono::milliseconds(delay));
            }
        }

        ClearDelayTasks();
    });
}

void IOContextThread::Stop() {
    stop();

    if (thread_.joinable()) {
        thread_.join();
    }
}

void IOContextThread::AddDelayTask(void *key, uint32_t delay, std::shared_ptr<TaskInterface> task) {
    boost::asio::dispatch(*this, [key, now = Now32() + delay, task, this]{
        delay_tasks_.emplace(now, std::make_pair(key, task));
        });
}

void IOContextThread::DelDelayTask(void *key) {
    boost::asio::dispatch(*this, [key, this] {
        auto it = delay_tasks_.begin();
        while (delay_tasks_.end() != it) {
            if (it->second.first == key) {
                it = delay_tasks_.erase(it);
            } else {
                ++it;
            }
        }
    });
}

bool IOContextThread::IsCurrentThread() const {
    return stopped() || thread_.get_id() == std::this_thread::get_id();
}

uint32_t IOContextThread::RunDelayTasks() {
    uint32_t now = Now32();

    while (!delay_tasks_.empty()) {
        auto task = delay_tasks_.begin()->second;
        if (now < delay_tasks_.begin()->first) {
            return delay_tasks_.begin()->first - now;
        }

        delay_tasks_.erase(delay_tasks_.begin());

        uint32_t delay = task.second->OnRun(now, false);
        if (TaskInterface::kNotContinue != delay) {
            delay_tasks_.emplace(now + delay, task);
        }
    }

    return 0;
}

void IOContextThread::ClearDelayTasks() {
    for (auto && task : delay_tasks_) {
        task.second.second->OnRun(0, true);
    }

    delay_tasks_.clear();
}


void AsioUDP::Start(UDPCallback *cb) {
    cb_ = cb;
    StartRead();
}

void AsioUDP::Stop() {
    socket_.shutdown(boost::asio::socket_base::shutdown_send);
    //socket_.cancel();
    socket_.close();
}

bool AsioUDP::Send(const UDPAddress& to, const char *buf, size_t len) {
    boost::asio::ip::udp::endpoint peer;
    Address2Endpoint(to, peer);

    //std::clog << "asio udp send to " << peer << std::endl;

    Write(WriteReq::New(peer, buf, len));

    return true;
}

const UDPAddress& AsioUDP::local_address() const {
    return address_;
}

ExecutorInterface *AsioUDP::executor() {
    return this;
}

// executor interface
bool AsioUDP::IsCurrentThread() const {
    return io_ctx_.IsCurrentThread();
}

bool AsioUDP::DispatchTask(std::shared_ptr<TaskInterface> task) {
    boost::asio::dispatch(socket_.get_executor(), [this, task] {
        uint32_t delay = task->OnRun(Now32(), false);
        if (TaskInterface::kNotContinue != delay) {
            io_ctx_.AddDelayTask(this, delay, task);
        }
    });

    return true;
}

AsioUDP::AsioUDP(IOContextThread& io_ctx, size_t recv_size)
    : socket_(io_ctx, boost::asio::ip::udp::v4())
    , recv_buf_(recv_size)
    , io_ctx_(io_ctx) {
    recv_buf_.shrink_to_fit();
}

AsioUDP::~AsioUDP() {
    io_ctx_.DelDelayTask(this);
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

    cb_->OnRecvUdp(from, recv_buf_.data(), bytes_transferred);
    StartRead();
}

bool AsioUDP::ErrorCallback(const boost::system::error_code& ec) {
    return cb_->OnError(ec.value(), ec.message().c_str());
}

AsioIOContext::AsioIOContext(size_t thread_num) : threads_(thread_num) {}

void AsioIOContext::Start() {
    for (auto&& thread : threads_) {
        thread.Start();
    }
}

void AsioIOContext::Stop() {
    for (auto&& thread : threads_) {
        thread.Stop();
    }
}

std::shared_ptr<UDPInterface>
AsioIOContext::CreateUDP(const UDPAddress& addr) {
    auto index = select_thread_index_++;

    std::shared_ptr<AsioUDP> udp{ 
        new AsioUDP(threads_[index % threads_.size()], 1024), 
        [](AsioUDP *p) { delete p; } 
    };

    if (!udp->Init(addr)) {
        return nullptr;
    }

    return udp;
}