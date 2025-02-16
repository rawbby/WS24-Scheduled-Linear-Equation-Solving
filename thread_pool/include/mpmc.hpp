#pragma once

#include <debug.hpp>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <stack>

template<typename T>
class MPMC
{
  std::stack<std::shared_ptr<T>> tasks_{};
  std::mutex tasks_mutexes_{};
  std::atomic_int32_t tasks_size_hint_{};

public:
  bool
  try_push(std::shared_ptr<T> task)
  {
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());

    std::unique_lock tasks_lock{ tasks_mutexes_, std::defer_lock };
    if (!tasks_lock.try_lock())
      return false;

    tasks_size_hint_.fetch_add(1, std::memory_order_release);
    tasks_.push(std::move(task));
    return true;
  }

  void
  push(std::shared_ptr<T> task)
  {
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());

    std::unique_lock tasks_lock{ tasks_mutexes_ };
    tasks_size_hint_.fetch_add(1, std::memory_order_release);
    tasks_.push(std::move(task));
  }

  std::optional<std::shared_ptr<T>>
  try_pop()
  {
    if (empty_hint())
      return std::nullopt;

    std::unique_lock tasks_lock{ tasks_mutexes_, std::defer_lock };
    if (!tasks_lock.try_lock())
      return std::nullopt;

    if (!tasks_.size())
      return std::nullopt;

    tasks_size_hint_.fetch_sub(1, std::memory_order_release);
    auto task = std::move(tasks_.top());
    tasks_.pop();
    tasks_lock.unlock();

    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());
    return task;
  }

  std::size_t
  size_hint() const
  {
    return tasks_size_hint_.load(std::memory_order_acquire);
  }

  bool
  empty_hint() const
  {
    return tasks_size_hint_.load(std::memory_order_acquire) == 0;
  }

  std::size_t
  size()
  {
    std::lock_guard lock{ tasks_mutexes_ };
    return tasks_.size();
  }

  bool
  empty()
  {
    std::lock_guard lock{ tasks_mutexes_ };
    return tasks_.empty();
  }
};
