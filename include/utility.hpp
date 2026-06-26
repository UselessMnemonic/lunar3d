#pragma once

#include <3ds.h>

#include <cstddef>
#include <limits>
#include <type_traits>

namespace lunar3d {

[[noreturn]] inline void panic(const char* message, size_t length) {
    svcOutputDebugString(message, static_cast<s32>(length));
    svcBreak(USERBREAK_PANIC);
    svcExitProcess();

    __builtin_unreachable();
}

template <size_t Length> [[noreturn]] inline void panic(const char (&message)[Length]) {
    static_assert(Length > 1, "Panic message must not be empty");

    panic(message, Length - 1);
}

template <typename T> class LinearBuffer {
  public:
    static_assert(std::is_trivial<T>::value, "LinearBuffer values must be POD types");

    LinearBuffer() = default;

    explicit LinearBuffer(size_t count) {
        reset(count);
    }

    ~LinearBuffer() {
        reset();
    }

    LinearBuffer(const LinearBuffer&) = delete;
    LinearBuffer& operator=(const LinearBuffer&) = delete;

    LinearBuffer(LinearBuffer&& other) noexcept : data_(other.data_), count_(other.count_) {
        other.data_ = nullptr;
        other.count_ = 0;
    }

    LinearBuffer& operator=(LinearBuffer&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        reset();

        data_ = other.data_;
        count_ = other.count_;
        other.data_ = nullptr;
        other.count_ = 0;

        return *this;
    }

    [[nodiscard]] T* get() noexcept {
        return data_;
    }

    [[nodiscard]] const T* get() const noexcept {
        return data_;
    }

    [[nodiscard]] T& operator[](size_t index) noexcept {
        return data_[index];
    }

    [[nodiscard]] const T& operator[](size_t index) const noexcept {
        return data_[index];
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return data_ != nullptr;
    }

    [[nodiscard]] size_t size() const noexcept {
        return count_;
    }

    [[nodiscard]] size_t bytes() const noexcept {
        return count_ * sizeof(T);
    }

    [[nodiscard]] T* release() noexcept {
        T* result = data_;
        data_ = nullptr;
        count_ = 0;
        return result;
    }

    void reset() noexcept {
        if (data_ != nullptr) {
            linearFree(data_);
            data_ = nullptr;
        }

        count_ = 0;
    }

    void reset(size_t count) {
        if (count == 0) {
            reset();
            return;
        }

        if (count > std::numeric_limits<size_t>::max() / sizeof(T)) {
            panic("lunar3d::LinearBuffer::reset: allocation size overflow");
        }

        T* data = static_cast<T*>(linearAlloc(count * sizeof(T)));
        if (data == nullptr) {
            panic("lunar3d::LinearBuffer::reset: linear allocation failed");
        }

        reset();
        data_ = data;
        count_ = count;
    }

  private:
    T* data_ = nullptr;
    size_t count_ = 0;
};

} // namespace lunar3d
