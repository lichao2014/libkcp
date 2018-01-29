#ifndef _ASIO_BUF_H_INCLUDED
#define _ASIO_BUF_H_INCLUDED

#include <unordered_map>

#include "boost/asio.hpp"

#include "common_types.h"

namespace kcp {
struct Buffer {
    Buffer *next;
    uint32_t capacity;
    uint32_t offset;
    char start[1];

    constexpr const char *data() const noexcept { return start + offset; };
    constexpr size_t len() const noexcept { return capacity - offset; };

    using Ptr = std::unique_ptr<Buffer, void (*)(const Buffer *)>;

    static Ptr New(size_t len, const char *data = nullptr, Buffer *next = nullptr) {
        return { NewRaw(len, data, next), FreeRaw };
    }

    static Ptr Create(Buffer *buf) {
        return { buf, FreeRaw };
    }

    static Buffer *NewRaw(size_t len, const char *data = nullptr, Buffer *next = nullptr);
    static void FreeRaw(const Buffer *buf);
};

class BufferList {
public:
    constexpr BufferList() noexcept : owned_(true), tail_(&head_) {}

    BufferList(const BufferList& rhs) 
        : owned_(false)
        , head_(rhs.head_)
        , tail_(rhs.tail_) {}

    ~BufferList() {
        if (owned_) {
            Clear();
        }
    }

    void Push(Buffer::Ptr buf) noexcept;
    Buffer::Ptr Pop() noexcept;
    void Clear() noexcept;
    bool Empty() const noexcept { return !head_; }
    bool Cut(size_t len);

    BufferList& operator =(BufferList&&) noexcept;

    class const_iterator 
        : public std::iterator_traits<boost::asio::const_buffer *> {
    public:
        explicit constexpr const_iterator(Buffer *buf = nullptr) : buf_(buf) {}

        value_type operator *() noexcept {
            return { buf_->data(), buf_->len() };
        }

        constexpr const_iterator& operator++() noexcept{
            buf_ = buf_->next;
            return *this;
        }

        constexpr const_iterator operator++(int) noexcept {
            return const_iterator(buf_->next);
        }

        constexpr bool operator!=(const_iterator& rhs) const noexcept {
            return buf_ != rhs.buf_;
        }
    protected:
        Buffer *buf_ = nullptr;
    };

    class iterator : public const_iterator {
    public:
        using const_iterator::const_iterator;
    };

    iterator begin() { return iterator(head_); }
    iterator end() { return iterator(); }

    const_iterator begin() const { return const_iterator(head_); }
    const_iterator end() const { return const_iterator(); }
private:
    Buffer *head_ = nullptr;
    Buffer **tail_ = nullptr;
    bool owned_ = false;
};

class WriteBuffers {
public:
    bool PutBuf(const IP4Address& peer, Buffer::Ptr buf);
    bool GetBufs(IP4Address *peer, BufferList *bufs);
private:
    std::unordered_map<IP4Address, BufferList, IP4Address::Hasher> by_address_bufs_;
};
}

#endif // !_ASIO_BUF_H_INCLUDED

