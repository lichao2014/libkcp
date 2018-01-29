#ifndef _ASIO_UDP_H_INCLUDED
#define _ASIO_UDP_H_INCLUDED

#include <memory>
#include <queue>
#include <vector>
#include <thread>
#include <atomic>
#include "boost/asio.hpp"

#include "udp_interface.h"
#include "asio_buf.h"

namespace kcp {
class IOContextThread : public boost::asio::io_context
                      , public ExecutorInterface {
public:
    IOContextThread() : thread_(), work_(*this), timer_(*this) {}
    ~IOContextThread() { Stop(); }

    void Start();
    void Stop();

    // executor interface
    bool CanNowExecuted() const override;
    bool DispatchTask(void *key, TaskInterface *task) override;
    void CancelTask(void *key) override;
private:
    uint32_t RunTasks();
    void CancelAllTasks();
    void StartTimer();
    void StopTimer();

    std::thread thread_;
    boost::asio::high_resolution_timer timer_;
    boost::asio::io_context::work work_;

    std::multimap<uint32_t, std::pair<void *, TaskInterface *>> tasks_;
};

class AsioUDP : public UDPInterface
              , public std::enable_shared_from_this<AsioUDP> {
public:
    // udp interface
    bool Open(UDPCallback *cb) override;
    void Close() override;
    void SetRecvBufSize(size_t recv_size) override;
    bool Send(const IP4Address& to, const char *buf, size_t len) override;
    const IP4Address& local_address() const override;
    ExecutorInterface *executor() override;
private:
    friend class AsioIOContext;

    explicit AsioUDP(std::shared_ptr<IOContextThread> io_ctx);
    ~AsioUDP() { Close(); }

    bool Bind(const IP4Address& addr);
    void TryStartWrite();
    void StartRead();
    void WriteCallback(std::size_t bytes_transferred);
    void ReadCallback(std::size_t bytes_transferred);
    bool ErrorCallback(const boost::system::error_code& ec);

    mutable boost::asio::ip::udp::socket socket_;
    WriteBuffers write_bufs_;
    BufferList in_writing_buf_;
    boost::asio::ip::udp::endpoint in_writing_peer_;

    std::vector<char> recv_buf_;
    boost::asio::ip::udp::endpoint peer_;
    size_t recv_buf_size_ = kDefaultRecvSize;

    std::shared_ptr<IOContextThread> io_ctx_;

    IP4Address address_;
    UDPCallback *cb_ = nullptr;
    bool in_writing_ = false;
    bool closed_ = true;
};

class AsioIOContext : public IOContextInterface {
public:
    static std::shared_ptr<AsioIOContext> Create(size_t thread_num);

    void Start() override;
    void Stop() override;
    std::shared_ptr<UDPInterface> CreateUDP(const IP4Address& addr) override;
    ExecutorInterface *executor() override;
private:
    explicit AsioIOContext(size_t thread_num);
    ~AsioIOContext() { Stop(); }

    const std::shared_ptr<IOContextThread>& SelectIOThread();

    std::vector<std::shared_ptr<IOContextThread>> threads_;
    std::atomic_size_t select_thread_index_ = 0;
};
}

#endif // !_ASIO_UDP_H_INCLUDED
