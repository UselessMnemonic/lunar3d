#pragma once

#include <3ds.h>

#include <cstddef>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>

namespace lunar3d {
namespace task {

namespace detail {

class LightLockGuard {
  public:
    explicit LightLockGuard(LightLock& lock) noexcept : lock_(&lock) {
        LightLock_Lock(lock_);
    }

    ~LightLockGuard() {
        unlock();
    }

    LightLockGuard(const LightLockGuard&) = delete;
    LightLockGuard& operator=(const LightLockGuard&) = delete;

    void unlock() noexcept {
        if (lock_ == nullptr) {
            return;
        }

        LightLock_Unlock(lock_);
        lock_ = nullptr;
    }

  private:
    LightLock* lock_;
};

[[noreturn]] inline void channelPanic(const char* message, size_t length) noexcept {
    svcOutputDebugString(message, static_cast<s32>(length));

    /*
     * Raise a panic-class user break.
     *
     * svcExitProcess() is a fallback in case a debugger resumes execution
     * after the break.
     */
    svcBreak(USERBREAK_PANIC);
    svcExitProcess();

    __builtin_unreachable();
}

template <size_t Length>
[[noreturn]] inline void channelPanic(const char (&message)[Length]) noexcept {
    static_assert(Length > 1, "Panic message must not be empty");

    channelPanic(message, Length - 1);
}

} // namespace detail

template <typename T, size_t Capacity> class Channel {
  public:
    static_assert(Capacity > 0, "Channel capacity must be greater than zero");

    static_assert(std::is_object<T>::value, "Channel values must be object types");

    /*
     * receive() must be able to construct a new T from the queued value.
     *
     * A normal copy constructor taking const T& can also satisfy construction
     * from an rvalue unless T has an explicitly deleted move constructor.
     */
    static_assert(std::is_constructible<T, T&&>::value || std::is_constructible<T, const T&>::value,
                  "Channel values must be move-constructible or copy-constructible");

    Channel() noexcept {
        LightLock_Init(&lock_);
        CondVar_Init(&canSend_);
        CondVar_Init(&canReceive_);
    }

    /*
     * Precondition:
     *
     * No thread may still be sending, receiving, or waiting on this channel.
     * close() wakes waiters, but does not wait for them to stop accessing the
     * Channel object.
     */
    ~Channel() noexcept {
        destroyBufferedValues();
    }

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    Channel(Channel&&) = delete;
    Channel& operator=(Channel&&) = delete;

    /*
     * Copy send.
     *
     * Available only when T can be constructed from const T&.
     * Blocks until capacity is available.
     */
    template <typename U = T>
    typename std::enable_if<std::is_constructible<U, const U&>::value, void>::type
    send(const T& value) noexcept {
        sendValue(value);
    }

    /*
     * Move send.
     *
     * Available only when T can be constructed from T&&.
     * Blocks until capacity is available.
     *
     * If this returns normally, value has been transferred to the channel.
     */
    template <typename U = T>
    typename std::enable_if<std::is_constructible<U, U&&>::value, void>::type
    send(T&& value) noexcept {
        sendValue(std::move(value));
    }

    /*
     * Nonblocking copy send.
     *
     * The copy is performed only after capacity has been confirmed.
     * Therefore, false leaves value untouched.
     */
    template <typename U = T>
    [[nodiscard]] typename std::enable_if<std::is_constructible<U, const U&>::value, bool>::type
    trySend(const T& value) noexcept {
        return trySendValue(value);
    }

    /*
     * Nonblocking move send.
     *
     * The move is performed only after capacity has been confirmed.
     * Therefore, false leaves value untouched, despite std::move being used
     * at the call site.
     */
    template <typename U = T>
    [[nodiscard]] typename std::enable_if<std::is_constructible<U, U&&>::value, bool>::type
    trySend(T&& value) noexcept {
        return trySendValue(std::move(value));
    }

    /*
     * Constructs T directly inside the channel.
     *
     * Blocks until capacity becomes available. Forwarded arguments are not
     * consumed while the channel remains full.
     */
    template <typename... Args>
    typename std::enable_if<std::is_constructible<T, Args&&...>::value, void>::type
    emplace(Args&&... args) noexcept {
        detail::LightLockGuard guard(lock_);

        while (count_ == Capacity && !closed_) {
            CondVar_Wait(&canSend_, &lock_);
        }

        if (closed_) {
            detail::channelPanic("lunar3d::task::Channel::emplace: closed channel");
        }

        constructAt(tail_, std::forward<Args>(args)...);

        tail_ = next(tail_);
        ++count_;

        guard.unlock();
        CondVar_Signal(&canReceive_);
    }

    /*
     * Attempts to construct T directly inside the channel.
     *
     * Returns false when full. In that case, no T is constructed and none of
     * the forwarded arguments are moved from.
     */
    template <typename... Args>
    [[nodiscard]] typename std::enable_if<std::is_constructible<T, Args&&...>::value, bool>::type
    tryEmplace(Args&&... args) noexcept {
        detail::LightLockGuard guard(lock_);

        if (closed_) {
            detail::channelPanic("lunar3d::task::Channel::tryEmplace: closed channel");
        }

        if (count_ == Capacity) {
            return false;
        }

        constructAt(tail_, std::forward<Args>(args)...);

        tail_ = next(tail_);
        ++count_;

        guard.unlock();
        CondVar_Signal(&canReceive_);

        return true;
    }

    /*
     * Blocks until:
     *
     * 1. A value becomes available, or
     * 2. The channel is closed and fully drained.
     *
     * nullopt unambiguously means closed and drained.
     */
    [[nodiscard]] std::optional<T> receive() noexcept {
        detail::LightLockGuard guard(lock_);

        while (count_ == 0 && !closed_) {
            CondVar_Wait(&canReceive_, &lock_);
        }

        if (count_ == 0) {
            return std::nullopt;
        }

        std::optional<T> result = takeHead();

        guard.unlock();
        CondVar_Signal(&canSend_);

        return result;
    }

    /*
     * Attempts to receive without blocking.
     *
     * nullopt means that no value was immediately available. It intentionally
     * does not distinguish open-and-empty from closed-and-drained.
     */
    [[nodiscard]] std::optional<T> tryReceive() noexcept {
        detail::LightLockGuard guard(lock_);

        if (count_ == 0) {
            return std::nullopt;
        }

        std::optional<T> result = takeHead();

        guard.unlock();
        CondVar_Signal(&canSend_);

        return result;
    }

    /*
     * Declares that no further sends may occur.
     *
     * Buffered values remain available to receivers. Closing an already
     * closed channel is treated as a producer-side ownership error.
     */
    void close() noexcept {
        {
            detail::LightLockGuard guard(lock_);

            if (closed_) {
                detail::channelPanic("lunar3d::task::Channel::close: already closed");
            }

            closed_ = true;
        }

        CondVar_Broadcast(&canSend_);
        CondVar_Broadcast(&canReceive_);
    }

    /*
     * Diagnostic snapshot only.
     *
     * The result must not be used as the predicate for a subsequent send or
     * receive because the channel may change immediately after this returns.
     */
    [[nodiscard]] size_t size() const noexcept {
        detail::LightLockGuard guard(lock_);
        return count_;
    }

    [[nodiscard]] static constexpr size_t capacity() noexcept {
        return Capacity;
    }

  private:
    struct Slot {
        alignas(T) unsigned char bytes[sizeof(T)];
    };

    template <typename U> void sendValue(U&& value) noexcept {
        detail::LightLockGuard guard(lock_);

        while (count_ == Capacity && !closed_) {
            CondVar_Wait(&canSend_, &lock_);
        }

        if (closed_) {
            detail::channelPanic("lunar3d::task::Channel::send: closed channel");
        }

        constructAt(tail_, std::forward<U>(value));

        tail_ = next(tail_);
        ++count_;

        guard.unlock();
        CondVar_Signal(&canReceive_);
    }

    template <typename U> [[nodiscard]] bool trySendValue(U&& value) noexcept {
        detail::LightLockGuard guard(lock_);

        if (closed_) {
            detail::channelPanic("lunar3d::task::Channel::trySend: closed channel");
        }

        if (count_ == Capacity) {
            return false;
        }

        /*
         * Forwarding occurs only after the capacity check. A failed
         * trySend(std::move(value)) therefore does not actually move value.
         */
        constructAt(tail_, std::forward<U>(value));

        tail_ = next(tail_);
        ++count_;

        guard.unlock();
        CondVar_Signal(&canReceive_);

        return true;
    }

    template <typename... Args> void constructAt(size_t index, Args&&... args) noexcept {
        ::new (static_cast<void*>(storage_[index].bytes)) T(std::forward<Args>(args)...);
    }

    [[nodiscard]] T* at(size_t index) noexcept {
        return std::launder(reinterpret_cast<T*>(storage_[index].bytes));
    }

    /*
     * Removes the current head while the caller holds lock_.
     *
     * Prefer construction from T&&. Fall back to const T& for types with an
     * explicitly deleted move constructor but an available copy constructor.
     */
    [[nodiscard]] std::optional<T> takeHead() noexcept {
        T* item = at(head_);

        std::optional<T> result;

        if constexpr (std::is_constructible<T, T&&>::value) {
            result.emplace(std::move(*item));
        } else {
            result.emplace(static_cast<const T&>(*item));
        }

        item->~T();

        head_ = next(head_);
        --count_;

        return result;
    }

    [[nodiscard]] static constexpr size_t next(size_t index) noexcept {
        return index + 1 == Capacity ? 0 : index + 1;
    }

    /*
     * Destruction requires exclusive ownership of the Channel object.
     * Consequently, no lock is acquired here.
     */
    void destroyBufferedValues() noexcept {
        while (count_ != 0) {
            at(head_)->~T();

            head_ = next(head_);
            --count_;
        }

        tail_ = head_;
    }

    mutable LightLock lock_;
    CondVar canSend_;
    CondVar canReceive_;

    Slot storage_[Capacity];

    size_t head_ = 0;
    size_t tail_ = 0;
    size_t count_ = 0;

    bool closed_ = false;
};

} // namespace task
} // namespace lunar3d
