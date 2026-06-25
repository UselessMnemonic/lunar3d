#pragma once

#include <3ds.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace lunar3d {
namespace task {

struct WorkerOptions {
    size_t stackSizeBytes = 32 * 1024;
    int threadPriority = 0x31;
    int processorAffinity = -2;
};

namespace detail {

template <size_t Length>
[[noreturn]] inline void workerPanic(const char (&message)[Length]) noexcept {
    static_assert(Length > 1, "Panic message must not be empty");

    svcOutputDebugString(message, static_cast<s32>(Length - 1));

    svcBreak(USERBREAK_PANIC);
    svcExitProcess();

    __builtin_unreachable();
}

inline void joinThread(::Thread& thread) noexcept {
    if (thread == nullptr) {
        workerPanic("lunar3d::task::Worker::join: worker already joined");
    }

    if (threadGetCurrent() == thread) {
        workerPanic("lunar3d::task::Worker::join: worker cannot join itself");
    }

    const Result result = threadJoin(thread, U64_MAX);

    if (R_FAILED(result)) {
        workerPanic("lunar3d::task::Worker::join: thread wait failed");
    }

    threadFree(thread);
    thread = nullptr;
}

inline bool tryJoinThread(::Thread& thread) noexcept {
    if (thread == nullptr) {
        workerPanic("lunar3d::task::Worker::tryJoin: worker already joined");
    }

    if (threadGetCurrent() == thread) {
        workerPanic("lunar3d::task::Worker::tryJoin: worker cannot join itself");
    }

    const Result result = threadJoin(thread, 0);

    if (result == 0) {
        threadFree(thread);
        thread = nullptr;
        return true;
    }

    if (R_DESCRIPTION(result) == RD_TIMEOUT) {
        return false;
    }

    workerPanic("lunar3d::task::Worker::tryJoin: thread wait failed");
}

} // namespace detail

template <typename TResult, typename TTask> class Worker {
  public:
    using Pointer = std::unique_ptr<Worker>;

    static_assert(std::is_object<TResult>::value, "Worker result must be an object type");

    static_assert(std::is_move_constructible<TResult>::value,
                  "Worker result must be move-constructible");

    static_assert(std::is_invocable_r<TResult, TTask&>::value,
                  "Worker task must be callable with no arguments "
                  "and produce TResult");

    template <typename Task>
    [[nodiscard]] static Pointer launch(Task&& task, WorkerOptions options = WorkerOptions{}) {
        Pointer worker{new Worker(std::forward<Task>(task))};

        if (!worker->start(options)) {
            return nullptr;
        }

        return worker;
    }

    ~Worker() {
        if (thread_ != nullptr) {
            detail::workerPanic("lunar3d::task::Worker::~Worker: worker not joined");
        }
    }

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

    Worker(Worker&&) = delete;
    Worker& operator=(Worker&&) = delete;

    [[nodiscard]] TResult join() {
        detail::joinThread(thread_);
        return takeResult();
    }

    [[nodiscard]] std::optional<TResult> tryJoin() {
        if (!detail::tryJoinThread(thread_)) {
            return std::nullopt;
        }

        return std::optional<TResult>{std::in_place, takeResult()};
    }

  private:
    template <typename Task> explicit Worker(Task&& task) : task_(std::forward<Task>(task)) {}

    bool start(const WorkerOptions& options) noexcept {
        thread_ = threadCreate(&Worker::threadEntry, this, options.stackSizeBytes,
                               options.threadPriority, options.processorAffinity, false);

        return thread_ != nullptr;
    }

    static void threadEntry(void* argument) {
        static_cast<Worker*>(argument)->run();
    }

    void run() {
        result_.emplace(std::invoke(task_));
    }

    TResult takeResult() {
        if (!result_) {
            detail::workerPanic("lunar3d::task::Worker::join: result already received");
        }

        TResult value(std::move(*result_));
        result_.reset();
        return value;
    }

    TTask task_;
    std::optional<TResult> result_;
    ::Thread thread_ = nullptr;
};

template <typename TTask> class Worker<void, TTask> {
  public:
    using Pointer = std::unique_ptr<Worker>;

    static_assert(std::is_invocable_r<void, TTask&>::value,
                  "Worker task must be callable with no arguments");

    template <typename Task>
    [[nodiscard]] static Pointer launch(Task&& task, WorkerOptions options = WorkerOptions{}) {
        Pointer worker{new Worker(std::forward<Task>(task))};

        if (!worker->start(options)) {
            return nullptr;
        }

        return worker;
    }

    ~Worker() {
        if (thread_ != nullptr) {
            detail::workerPanic("lunar3d::task::Worker::~Worker: worker not joined");
        }
    }

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

    Worker(Worker&&) = delete;
    Worker& operator=(Worker&&) = delete;

    void join() {
        detail::joinThread(thread_);
    }

    [[nodiscard]] bool tryJoin() {
        return detail::tryJoinThread(thread_);
    }

  private:
    template <typename Task> explicit Worker(Task&& task) : task_(std::forward<Task>(task)) {}

    bool start(const WorkerOptions& options) noexcept {
        thread_ = threadCreate(&Worker::threadEntry, this, options.stackSizeBytes,
                               options.threadPriority, options.processorAffinity, false);

        return thread_ != nullptr;
    }

    static void threadEntry(void* argument) {
        static_cast<Worker*>(argument)->run();
    }

    void run() {
        std::invoke(task_);
    }

    TTask task_;
    ::Thread thread_ = nullptr;
};

template <typename Task>
[[nodiscard]] auto launchWorker(Task&& task, WorkerOptions options = WorkerOptions{}) {
    using StoredTask = typename std::decay<Task>::type;

    using Result = typename std::invoke_result<StoredTask&>::type;

    static_assert(std::is_void<Result>::value || std::is_object<Result>::value,
                  "Worker task must return void or an object by value");

    return Worker<Result, StoredTask>::launch(std::forward<Task>(task), options);
}

template <typename Task>
[[nodiscard]] auto launch(Task&& task, WorkerOptions options = WorkerOptions{}) {
    using StoredTask = typename std::decay<Task>::type;
    using Result = typename std::invoke_result<StoredTask&>::type;

    return Worker<Result, StoredTask>::launch(std::forward<Task>(task), options);
}

} // namespace task
} // namespace lunar3d
