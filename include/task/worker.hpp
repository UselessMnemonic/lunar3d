#pragma once

#include <3ds.h>

#include <cstddef>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

namespace lunar3d {
namespace task {

template <typename TResult, typename TRunnable = std::function<TResult()>> class Worker {
  public:
    explicit Worker(TRunnable runnable, size_t stackSizeBytes = 32 * 1024,
                    int threadPriority = 0x31, int processorAffinity = -2)
        : runnable_(std::move(runnable)) {
        LightLock_Init(&lock_);
        thread_ = threadCreate(threadEntry, this, stackSizeBytes, threadPriority, processorAffinity,
                               false);
    }

    ~Worker() {
        join();
        destroyResult();
    }

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

    bool started() const {
        return thread_ != nullptr;
    }

    bool completed() const {
        LightLock_Lock(&lock_);
        const bool result = completed_;
        LightLock_Unlock(&lock_);
        return result;
    }

    bool takeResult(TResult& result) {
        LightLock_Lock(&lock_);
        if (!hasResult_) {
            LightLock_Unlock(&lock_);
            return false;
        }

        result = std::move(*storedResult());
        storedResult()->~TResult();
        hasResult_ = false;
        LightLock_Unlock(&lock_);
        return true;
    }

    void join() {
        if (!thread_) {
            return;
        }

        threadJoin(thread_, U64_MAX);
        threadFree(thread_);
        thread_ = nullptr;
    }

  private:
    static void threadEntry(void* argument) {
        static_cast<Worker*>(argument)->run();
    }

    void run() {
        TResult result = runRunnable();

        LightLock_Lock(&lock_);
        new (&result_) TResult(std::move(result));
        hasResult_ = true;
        completed_ = true;
        LightLock_Unlock(&lock_);
    }

    TResult* storedResult() {
        return reinterpret_cast<TResult*>(&result_);
    }

    void destroyResult() {
        LightLock_Lock(&lock_);
        if (hasResult_) {
            storedResult()->~TResult();
            hasResult_ = false;
        }
        LightLock_Unlock(&lock_);
    }

    TResult runRunnable() {
        if constexpr (requires(TRunnable & runnable) { runnable.run(); }) {
            return runnable_.run();
        } else {
            return runnable_();
        }
    }

    TRunnable runnable_;
    typename std::aligned_storage<sizeof(TResult), alignof(TResult)>::type result_;
    mutable LightLock lock_;
    ::Thread thread_ = nullptr;
    bool completed_ = false;
    bool hasResult_ = false;
};

template <typename TRunnable> class Worker<void, TRunnable> {
  public:
    explicit Worker(TRunnable runnable, size_t stackSizeBytes = 32 * 1024,
                    int threadPriority = 0x31, int processorAffinity = -2)
        : runnable_(std::move(runnable)) {
        LightLock_Init(&lock_);
        thread_ = threadCreate(threadEntry, this, stackSizeBytes, threadPriority, processorAffinity,
                               false);
    }

    ~Worker() {
        join();
    }

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

    bool started() const {
        return thread_ != nullptr;
    }

    bool completed() const {
        LightLock_Lock(&lock_);
        const bool result = completed_;
        LightLock_Unlock(&lock_);
        return result;
    }

    void join() {
        if (!thread_) {
            return;
        }

        threadJoin(thread_, U64_MAX);
        threadFree(thread_);
        thread_ = nullptr;
    }

  private:
    static void threadEntry(void* argument) {
        static_cast<Worker*>(argument)->run();
    }

    void run() {
        runRunnable();

        LightLock_Lock(&lock_);
        completed_ = true;
        LightLock_Unlock(&lock_);
    }

    void runRunnable() {
        if constexpr (requires(TRunnable & runnable) { runnable.run(); }) {
            runnable_.run();
        } else {
            runnable_();
        }
    }

    TRunnable runnable_;
    mutable LightLock lock_;
    ::Thread thread_ = nullptr;
    bool completed_ = false;
};

} // namespace task
} // namespace lunar3d
