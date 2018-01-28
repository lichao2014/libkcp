#ifndef _LOCK_PTR_H_INCLUDED
#define _LOCK_PTR_H_INCLUDED

#include <mutex>
#include <shared_mutex>

namespace kcp {
template<typename T, typename Guard>
class ScopedLockPtr : public Guard {
public:
    template<typename Mutex>
    ScopedLockPtr(Mutex& mu, T *ptr) : Guard(mu), ptr_(ptr) {}

    T *operator->() {
        return ptr_;
    }

    const T *operator->() const {
        return ptr_;
    }

private:
    T *ptr_;
};

struct UseUniqueLock {
    template<typename Mutex>
    using Guard = std::unique_lock<Mutex>;
};

struct UseSharedLock {
    template<typename Mutex>
    using Guard = std::shared_lock<Mutex>;
};

constexpr UseUniqueLock use_unique_lock;
constexpr UseSharedLock use_shared_lock;

template<typename T, typename Mutex>
class LockPtr {
public:
    template<typename ... Args>
    LockPtr(Args&& ... args) : value_(std::forward<Args>(args)...) {}

    template<typename LockTraits>
    ScopedLockPtr<T, typename LockTraits::template Guard<Mutex>>
    get(LockTraits lock_traits) {
        return { mu_, &value_ };
    }

    template<typename LockTraits>
    const ScopedLockPtr<T, typename LockTraits::template Guard<Mutex>>
    get(LockTraits lock_traits) const {
        return { mu_, &value_ };
    }

    auto operator->() {
        return get(use_unique_lock);
    }

    auto operator->() const {
        return get(use_unique_lock);
    }

private:
    T value_;
    Mutex mu_;
};
}
#endif // !_LOCK_PTR_H_INCLUDED