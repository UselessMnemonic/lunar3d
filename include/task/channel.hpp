#pragma once

#include <3ds.h>

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace lunar3d {
namespace task {

template <typename T, size_t Capacity>
class Channel {
public:
    static_assert(Capacity > 0, "Channel capacity must be greater than zero");

    Channel() {
        LightLock_Init(&lock_);
        CondVar_Init(&canSend_);
        CondVar_Init(&canReceive_);
    }

    ~Channel() {
        close();
        clear();
    }

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    bool send(const T& value) {
        return sendValue(value);
    }

    bool send(T&& value) {
        return sendValue(std::move(value));
    }

    bool trySend(const T& value) {
        return trySendValue(value);
    }

    bool trySend(T&& value) {
        return trySendValue(std::move(value));
    }

    bool receive(T& value) {
        LightLock_Lock(&lock_);
        while (count_ == 0 && active_) {
            CondVar_Wait(&canReceive_, &lock_);
        }

        if (count_ == 0 && !active_) {
            LightLock_Unlock(&lock_);
            return false;
        }

        popInto(value);
        CondVar_Signal(&canSend_);
        LightLock_Unlock(&lock_);
        return true;
    }

    bool tryReceive(T& value) {
        LightLock_Lock(&lock_);
        if (count_ == 0) {
            LightLock_Unlock(&lock_);
            return false;
        }

        popInto(value);
        CondVar_Signal(&canSend_);
        LightLock_Unlock(&lock_);
        return true;
    }

    void close() {
        LightLock_Lock(&lock_);
        if (active_) {
            active_ = false;
            CondVar_Broadcast(&canSend_);
            CondVar_Broadcast(&canReceive_);
        }
        LightLock_Unlock(&lock_);
    }

    size_t size() const {
        LightLock_Lock(&lock_);
        const size_t result = count_;
        LightLock_Unlock(&lock_);
        return result;
    }

    constexpr size_t capacity() const {
        return Capacity;
    }

private:
    using Storage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

    T* at(size_t index) {
        return reinterpret_cast<T*>(&storage_[index]);
    }

    const T* at(size_t index) const {
        return reinterpret_cast<const T*>(&storage_[index]);
    }

    template <typename Value>
    bool sendValue(Value&& value) {
        LightLock_Lock(&lock_);
        while (count_ == Capacity && active_) {
            CondVar_Wait(&canSend_, &lock_);
        }

        if (!active_) {
            LightLock_Unlock(&lock_);
            return false;
        }

        push(std::forward<Value>(value));
        CondVar_Signal(&canReceive_);
        LightLock_Unlock(&lock_);
        return true;
    }

    template <typename Value>
    bool trySendValue(Value&& value) {
        LightLock_Lock(&lock_);
        if (!active_ || count_ == Capacity) {
            LightLock_Unlock(&lock_);
            return false;
        }

        push(std::forward<Value>(value));
        CondVar_Signal(&canReceive_);
        LightLock_Unlock(&lock_);
        return true;
    }

    template <typename Value>
    void push(Value&& value) {
        new (&storage_[tail_]) T(std::forward<Value>(value));
        tail_ = (tail_ + 1) % Capacity;
        ++count_;
    }

    void popInto(T& value) {
        T* item = at(head_);
        value = std::move(*item);
        item->~T();
        head_ = (head_ + 1) % Capacity;
        --count_;
    }

    void clear() {
        LightLock_Lock(&lock_);
        while (count_ > 0) {
            at(head_)->~T();
            head_ = (head_ + 1) % Capacity;
            --count_;
        }
        tail_ = head_;
        LightLock_Unlock(&lock_);
    }

    mutable LightLock lock_;
    CondVar canSend_;
    CondVar canReceive_;
    Storage storage_[Capacity];
    size_t head_ = 0;
    size_t tail_ = 0;
    size_t count_ = 0;
    bool active_ = true;
};

} // namespace task
} // namespace lunar3d
