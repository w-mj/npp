//
// Created by benny on 2022/3/17.
//

#ifndef CPPCOROUTINES_04_TASK_TASK_H_
#define CPPCOROUTINES_04_TASK_TASK_H_

#include "coroutine_common.h"
#include "TaskPromise.h"
#include "ElasticExecutor.h"
#include <type_traits>

template<typename T> concept not_void = !std::is_void_v<T>;

template<typename ResultType, typename Executor = NPP::SharedElasticExecutor>
struct Task {
  using promise_type = TaskPromise<ResultType, Executor>;

  auto as_awaiter() {
    return TaskAwaiter<ResultType, Executor>(std::move(*this));
  }

  ResultType get_result() requires (!std::is_void_v<ResultType>) {
    return handle.promise().get_result();
  }

  void get_result() requires (std::is_void_v<ResultType>) {
      handle.promise().get_result();
  }

  template<typename T>
  requires (!std::is_void_v<ResultType> && std::is_same_v<T, ResultType>)
  Task &then(std::function<void(T)> &&func)  {
    handle.promise().on_completed([func](auto result) {
      try {
        func(result.get_or_throw());
      } catch (std::exception &e) {
        // ignore.
      }
    });
    return *this;
  }

  template<typename=void>
  requires std::is_void_v<ResultType>
  Task &then(std::function<void()> &&func)  {
      handle.promise().on_completed([func](auto result) {
          try {
              result.get_or_throw();
              func();
          } catch (std::exception &e) {
              // ignore.
          }
      });
      return *this;
  }

  Task &catching(std::function<void(std::exception &)> &&func) {
    handle.promise().on_completed([func](auto result) {
      try {
        result.get_or_throw();
      } catch (std::exception &e) {
        func(e);
      }
    });
    return *this;
  }

  Task &finally(std::function<void()> &&func) {
    handle.promise().on_completed([func](auto result) { func(); });
    return *this;
  }

  explicit Task(std::coroutine_handle<promise_type> handle) noexcept: handle(handle) {}
  Task() = default;

  Task(Task &&task) noexcept: handle(std::exchange(task.handle, {})) {}

  Task(Task &) = delete;

  Task &operator=(Task &) = delete;

  Task &operator=(Task &&task)  noexcept {
      handle = std::exchange(task.handle, {});
      return *this;
  }

  ~Task() {
    if (handle) handle.destroy();
  }

 private:
  std::coroutine_handle<promise_type> handle;
};


#endif //CPPCOROUTINES_04_TASK_TASK_H_
