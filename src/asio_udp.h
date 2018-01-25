#ifndef _ASIO_UDP_H_INCLUDED
#define _ASIO_UDP_H_INCLUDED

#include <memory>
#include <queue>
#include <vector>
#include <thread>
#include <atomic>
#include "boost/asio.hpp"

#include "udp_interface.h"

namespace kcp {
struct WriteReq {
public:
    using Ptr = std::unique_ptr<WriteReq, void(*)(WriteReq *)>;

    boost::asio::ip::udp::endpoint peer;
    size_t offset;
    size_t size;
    char data[1];

    char *buf() { return data + offset; }

    const char *buf() const { return data + offset; }
    size_t len() const { return size - offset; }

    static Ptr New(const boost::asio::ip::udp::endpoint& peer, const char *buf, std::size_t len) {
        return { NewRaw(peer, buf, len), &WriteReq::FreeRaw };
    }

    static WriteReq *NewRaw(const boost::asio::ip::udp::endpoint& peer, const char *buf, std::size_t len) {
        WriteReq *req = reinterpret_cast<WriteReq *>(malloc(sizeof(WriteReq) + len));
        if (req) {
            req->peer = peer;
            req->offset = 0;
            req->size = len;
            memcpy(req->data, buf, len);
        }

        return req;
    }

    static void FreeRaw(WriteReq *req) {
        free(req);
    }
};

class IOContextThread : public boost::asio::io_context {
public:
    IOContextThread() : thread_(), work_(*this) {}

    void Start();
    void Stop();

    void AddDelayTask(void *key, uint32_t delay, std::shared_ptr<TaskInterface> task);
    void DelDelayTask(void *key);

    bool IsCurrentThread() const;
private:
    uint32_t RunDelayTasks();
    void ClearDelayTasks();

    std::thread thread_;
    boost::asio::io_context::work work_;

    using TaskKeyPair = std::pair<void *, std::shared_ptr<TaskInterface>>;
    std::multimap<uint32_t, TaskKeyPair> delay_tasks_;
};

class AsioUDP : public UDPInterface
              , public ExecutorInterface
              , public std::enable_shared_from_this<AsioUDP> {
public:
    // udp interface
    void Start(UDPCallback *cb) override;
    void Stop() override;
    bool Send(const UDPAddress& to, const char *buf, size_t len) override;
    const UDPAddress& local_address() const override;
    ExecutorInterface *executor() override;

    // executor interface
    bool IsCurrentThread() const override;
    bool DispatchTask(std::shared_ptr<TaskInterface> task) override;
private:
    friend class AsioIOContext;

    AsioUDP(IOContextThread& io_ctx, size_t recv_size);
    ~AsioUDP();

    bool Init(const UDPAddress& addr);

    void Write(WriteReq::Ptr req);
    void StartWrite();
    void StartRead();
    void WriteCallback(WriteReq::Ptr req, std::size_t bytes_transferred);
    void ReadCallback(std::size_t bytes_transferred);
    bool ErrorCallback(const boost::system::error_code& ec);

    std::queue<WriteReq::Ptr> write_req_queue_;
    bool in_writing_ = false;

    std::vector<char> recv_buf_;

    boost::asio::ip::udp::endpoint peer_;
    mutable boost::asio::ip::udp::socket socket_;
    UDPAddress address_;

    IOContextThread& io_ctx_;
    UDPCallback *cb_ = nullptr;
};

class AsioIOContext : public IOContextInterface {
public:
    explicit AsioIOContext(size_t thread_num);

    void Start() override;
    void Stop() override;
    std::shared_ptr<UDPInterface> CreateUDP(const UDPAddress& addr) override;
    ExecutorInterface *executor() override { return nullptr; }
private:
    std::vector<IOContextThread> threads_;
    std::atomic_size_t select_thread_index_ = 0;
};
}

#endif // !_ASIO_UDP_H_INCLUDED