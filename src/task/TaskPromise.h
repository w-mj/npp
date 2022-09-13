//
// Created by benny on 2022/3/17.
//

#ifndef CPPCOROUTINES_TASKS_04_TASK_TASKPROMISE_H_
#define CPPCOROUTINES_TASKS_04_TASK_TASKPROMISE_H_

#include <functional>
#include <mutex>
#include <list>
#include <optional>

#include "coroutine"
#include "Result.h"
#include "DispatchAwaiter.h"
#include "TaskAwaiter.h"
#include "SleepAwaiter.h"
#include "ChannelAwaiter.h"
#include "CommonAwaiter.h"
#include "basic/Logger.h"

namespace NPP {

    template<typename AwaiterImpl, typename R>
    concept AwaiterImplRestriction = std::is_base_of<Awaiter<R>, AwaiterImpl>::value;

    template<typename ResultType, typename Executor>
    class Task;

    template<typename ResultType, typename Executor>
    struct TaskPromiseBase {

        DispatchAwaiter initial_suspend() { return DispatchAwaiter{&executor}; }

        std::suspend_always final_suspend() noexcept { return {}; }


        template<typename _ResultType, typename _Executor>
        TaskAwaiter<_ResultType, _Executor> await_transform(Task<_ResultType, _Executor> &&task) {
            return await_transform(TaskAwaiter<_ResultType, _Executor>(std::move(task)));
        }

        template<typename _Rep, typename _Period>
        auto await_transform(std::chrono::duration<_Rep, _Period> duration) {
            return await_transform(SleepAwaiter(duration));
        }

        template<typename AwaiterImpl>
        requires AwaiterImplRestriction<AwaiterImpl, typename AwaiterImpl::ResultType>
        AwaiterImpl await_transform(AwaiterImpl awaiter) {
            awaiter.install_executor(&executor);
            return awaiter;
        }

        void unhandled_exception() {
            std::lock_guard lock(completion_lock);
            result = Result<ResultType>(std::current_exception());
            completion.notify_all();
            notify_callbacks();
        }


        ResultType get_result() {
            // blocking for result or throw on exception
            std::unique_lock lock(completion_lock);
            if (!result.has_value()) {
                completion.wait(lock);
            }
            return result->get_or_throw();
        }

        void on_completed(std::function<void(Result<ResultType>)> &&func) {
            std::unique_lock lock(completion_lock);
            if (result.has_value()) {
                auto value = result.value();
                lock.unlock();
                func(value);
            } else {
                completion_callbacks.push_back(func);
            }
        }

        bool entirely_finish() {
            return result.has_value() && completion_callbacks.empty();
        }

    protected:
        std::optional<Result<ResultType>> result;

        std::mutex completion_lock;
        std::condition_variable completion;

        std::list<std::function<void(Result<ResultType>)>> completion_callbacks;

        Executor executor;

        void notify_callbacks() {
            auto value = result.value();
            for (auto &callback: completion_callbacks) {
                callback(value);
            }
            completion_callbacks.clear();
        }
    };

    template<typename ResultType, typename Executor>
    class TaskPromise : public TaskPromiseBase<ResultType, Executor> {
        using Base = TaskPromiseBase<ResultType, Executor>;
    public:
        Task<ResultType, Executor> get_return_object() {
            return Task{std::coroutine_handle<TaskPromise>::from_promise(*this)};
        }

        void return_value(ResultType value) {
            std::lock_guard lock(Base::completion_lock);
            Base::result = Result<ResultType>(std::move(value));
            Base::completion.notify_all();
            Base::notify_callbacks();
        }
    };

    template<typename Executor>
    class TaskPromise<void, Executor> : public TaskPromiseBase<void, Executor> {
        using Base = TaskPromiseBase<void, Executor>;
    public:
        Task<void, Executor> get_return_object() {
            return Task{std::coroutine_handle<TaskPromise>::from_promise(*this)};
        }

        void return_void() {
            std::lock_guard lock(Base::completion_lock);
            Base::result = Result<void>();
            Base::completion.notify_all();
            Base::notify_callbacks();
        }

    };
}

#endif //CPPCOROUTINES_TASKS_04_TASK_TASKPROMISE_H_
