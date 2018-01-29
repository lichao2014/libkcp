#include "asio_buf.h"

using namespace kcp;

// static 
Buffer *Buffer::NewRaw(size_t len, const char *data, Buffer *next) {
    Buffer *buf = reinterpret_cast<Buffer *>(new char[sizeof(Buffer) + len]);
    if (buf) {
        buf->capacity = static_cast<uint32_t>(len);
        buf->offset = 0;
        buf->next = next;

        if (data) {
            memcpy(buf->start, data, len);
        }
    }
    return buf;
}

// static 
void Buffer::FreeRaw(const Buffer *buf) {
    delete [] reinterpret_cast<const char *>(buf);
}

void BufferList::Push(Buffer::Ptr buf) noexcept {
    *tail_ = buf.get();
    tail_ = &(buf->next);
    buf.release();
}

Buffer::Ptr BufferList::Pop() noexcept {
    auto buf = head_;
    if (head_) {
        head_ = head_->next;
    }
    return Buffer::Create(buf);
}

void BufferList::Clear() noexcept {
    while (head_) {
        auto buf = head_;
        head_ = head_->next;
        Buffer::FreeRaw(buf);
    }
}

BufferList& BufferList::operator =(BufferList&& rhs) noexcept {
    if (this != &rhs) {
        std::swap(head_, rhs.head_);
        std::swap(tail_, rhs.tail_);
        std::swap(owned_, rhs.owned_);

        if (!head_) {
            tail_ = &head_;
        }

        if (!rhs.head_) {
            rhs.tail_ = &rhs.head_;
        }
    }

    return *this;
}

bool BufferList::Cut(size_t len) {
    while (head_ && head_->len() <= len) {
        auto buf = head_;
        if (buf->len() > len) {
            buf->offset += static_cast<uint32_t>(len);
            return true;
        }

        head_ = head_->next;

        len -= buf->len();
        Buffer::FreeRaw(buf);
    }

    return false;
}

bool WriteBuffers::PutBuf(const IP4Address& peer, Buffer::Ptr buf) {
    by_address_bufs_[peer].Push(std::move(buf));
    return true;
}

bool WriteBuffers::GetBufs(IP4Address *peer, BufferList *bufs) {
    for (auto&& [k, v] : by_address_bufs_) {
        if (!v.Empty()) {
            *peer = k;
            *bufs = std::move(v);
            return true;
        }
    }

    return false;
}