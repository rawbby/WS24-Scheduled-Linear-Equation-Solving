#pragma once

#include <debug.hpp>
#include <util.hpp>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <stack>
#include <thread>
#include <vector>

class Task
{
  std::atomic<bool> finished_{ false };

public:
  virtual ~Task() = default;

  virtual void
  operator()(std::size_t tid) = 0;

  void
  run(std::size_t tid)
  {
    DEBUG_ASSERT(!finished(), "task already performed. tasks are not reusable!");
    this->operator()(tid);
    finished_.store(true, std::memory_order_release);
  }

  bool
  finished() const
  {
    return finished_.load(std::memory_order_acquire);
  }
};

#define TASK_INIT_PACKED(PACKED_ARG) TAIL(UNPACK(PACKED_ARG))(std::move(TAIL(UNPACK(PACKED_ARG))))

#define TASK(NAME, ...)                                                    \
  using NAME = struct NAME final : Task                                    \
  {                                                                        \
    JOIN(;, FOR_EACH(JOIN_UNPACK, POP(__VA_ARGS__)));                      \
                                                                           \
    NAME(FOR_EACH(JOIN_UNPACK, POP(__VA_ARGS__)))                          \
      : Task() __VA_OPT__(, ) FOR_EACH(TASK_INIT_PACKED, POP(__VA_ARGS__)) \
    {                                                                      \
    }                                                                      \
                                                                           \
    ~NAME() override = default;                                            \
                                                                           \
    void                                                                   \
    operator()(std::size_t tid) override                                   \
    {                                                                      \
      HEAD(REVERSE(__VA_ARGS__));                                          \
    }                                                                      \
  }

class TaskStack
{
  std::stack<std::shared_ptr<Task>> tasks_{};
  std::mutex tasks_mutexes_{};
  std::atomic_int32_t tasks_size_hint_{};

public:
  bool
  try_push(std::shared_ptr<Task> task)
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
  push(std::shared_ptr<Task> task)
  {
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());

    std::unique_lock tasks_lock{ tasks_mutexes_ };
    tasks_size_hint_.fetch_add(1, std::memory_order_release);
    tasks_.push(std::move(task));
  }

  std::optional<std::shared_ptr<Task>>
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

class FixedTaskStack
{
  std::size_t capacity_;
  std::stack<std::shared_ptr<Task>> tasks_{};
  std::mutex tasks_mutexes_{};
  std::atomic_int32_t tasks_size_hint_{};

public:
  explicit FixedTaskStack(std::size_t capacity)
    : capacity_(capacity)
  {
  }

  FixedTaskStack()
    : capacity_(0)
  {
  }

  void
  set_capacity(std::size_t capacity)
  {
    capacity_ = capacity;
  }

  bool
  try_push(std::shared_ptr<Task> task)
  {
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());

    if (full_hint())
      return false;

    std::unique_lock tasks_lock{ tasks_mutexes_, std::defer_lock };
    if (!tasks_lock.try_lock())
      return false;

    tasks_size_hint_.fetch_add(1, std::memory_order_release);
    tasks_.push(std::move(task));
    return true;
  }

  std::optional<std::shared_ptr<Task>>
  try_pop()
  {
    if (empty_hint())
      return std::nullopt;

    std::unique_lock tasks_lock{ tasks_mutexes_, std::defer_lock };
    if (!tasks_lock.try_lock())
      return std::nullopt;

    if (tasks_.empty())
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

  bool
  full_hint() const
  {
    return tasks_size_hint_.load(std::memory_order_acquire) >= capacity_;
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

  bool
  full()
  {
    std::lock_guard lock{ tasks_mutexes_ };
    return tasks_.size() >= capacity_;
  }
};

class StealTaskStack
{
  // one fixed stack per thread for sub-task
  // or tasks with thread local data
  std::vector<FixedTaskStack> local_tasks_;

  // one stack per thread for fast task
  // acquisition in case there are many tasks
  std::vector<TaskStack> overflow_tasks_;

  // one fixed stack for new
  // tasks without locality
  FixedTaskStack tasks_;

  // buffer the combined
  // size of all buffers
  std::atomic_int32_t size_;

public:
  explicit StealTaskStack(std::size_t num_threads)
    : local_tasks_(num_threads)
    , overflow_tasks_(num_threads)
    , tasks_(num_threads)
    , size_(0)
  {
    for (std::size_t i = 0; i < num_threads; ++i)
      local_tasks_[i].set_capacity(1024);
  }

  void
  push(std::shared_ptr<Task> task, std::size_t tid)
  {
    size_.fetch_add(1, std::memory_order_release);
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());
    DEBUG_ASSERT_LT(tid, local_tasks_.size());
    if (!local_tasks_[tid].try_push(task))
      overflow_tasks_[tid].push(std::move(task));
  }

  void
  push(std::shared_ptr<Task> task)
  {
    size_.fetch_add(1, std::memory_order_release);
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());
    if (tasks_.try_push(task))
      return;
    auto min_i = std::numeric_limits<std::size_t>::min();
    auto min_s = std::numeric_limits<std::size_t>::max();
    for (std::size_t i = 0; i < local_tasks_.size(); ++i) {
      const auto s = overflow_tasks_[i].size_hint();
      if (!s)
        return overflow_tasks_[i].push(std::move(task));
      if (s < min_s) {
        min_i = i;
        min_s = s;
      }
    }
    overflow_tasks_[min_i].push(std::move(task));
  }

  std::optional<std::shared_ptr<Task>>
  try_pop(std::size_t tid)
  {
    auto task = local_tasks_[tid].try_pop();
    if (task.has_value()) {
      DEBUG_ASSERT_NE(task.value(), nullptr);
      DEBUG_ASSERT(!task.value()->finished());
      size_.fetch_sub(1, std::memory_order_release);
      return task.value();
    }
    task = overflow_tasks_[tid].try_pop();
    if (task.has_value()) {
      DEBUG_ASSERT_NE(task.value(), nullptr);
      DEBUG_ASSERT(!task.value()->finished());
      size_.fetch_sub(1, std::memory_order_release);
      return task.value();
    }
    if (empty())
      return std::nullopt;
    task = tasks_.try_pop();
    if (task.has_value()) {
      DEBUG_ASSERT_NE(task.value(), nullptr);
      DEBUG_ASSERT(!task.value()->finished());
      size_.fetch_sub(1, std::memory_order_release);
      return task.value();
    }
    for (std::size_t i = tid + 1; i < local_tasks_.size(); ++i) {
      task = overflow_tasks_[i].try_pop();
      if (task.has_value()) {
        DEBUG_ASSERT_NE(task.value(), nullptr);
        DEBUG_ASSERT(!task.value()->finished());
        size_.fetch_sub(1, std::memory_order_release);
        return task.value();
      }
    }
    for (std::size_t i = 0; i < tid; ++i) {
      task = overflow_tasks_[i].try_pop();
      if (task.has_value()) {
        DEBUG_ASSERT_NE(task.value(), nullptr);
        DEBUG_ASSERT(!task.value()->finished());
        size_.fetch_sub(1, std::memory_order_release);
        return task.value();
      }
    }
    for (std::size_t i = tid + 1; i < local_tasks_.size(); ++i) {
      task = local_tasks_[i].try_pop();
      if (task.has_value()) {
        DEBUG_ASSERT_NE(task.value(), nullptr);
        DEBUG_ASSERT(!task.value()->finished());
        size_.fetch_sub(1, std::memory_order_release);
        return task.value();
      }
    }
    for (std::size_t i = 0; i < tid; ++i) {
      task = local_tasks_[i].try_pop();
      if (task.has_value()) {
        DEBUG_ASSERT_NE(task.value(), nullptr);
        DEBUG_ASSERT(!task.value()->finished());
        size_.fetch_sub(1, std::memory_order_release);
        return task.value();
      }
    }
    return std::nullopt;
  }

  std::size_t
  size() const
  {
    return size_.load(std::memory_order_acquire);
  }

  bool
  empty() const
  {
    return size_.load(std::memory_order_acquire) == 0;
  }
};

class ThreadPool
{
public:
  explicit ThreadPool(std::size_t num_threads = std::max(1u, std::thread::hardware_concurrency() - 1))
    : num_threads(num_threads)
    , stop_flag_(false)
    , working_count_(0)
    , tasks_(num_threads)
    , meta_sum_elapsed_run_(num_threads)
    , meta_elapsed_(num_threads)
  {
    for (std::size_t i = 0; i < num_threads; ++i) {
      workers_.emplace_back([this, i] {
        double sum_elapsed_run = 0.0;
        const auto start = std::chrono::steady_clock::now();
        while (!(stop_flag_ && !working_count_ && tasks_.empty())) {
          working_count_.fetch_add(1);
          auto optional_task = tasks_.try_pop(i);
          if (optional_task.has_value()) {
            const auto task = optional_task.value();
            DEBUG_ASSERT_NE(task, nullptr);
            DEBUG_ASSERT(!task->finished());
            const auto start_run = std::chrono::steady_clock::now();
            task->run(i);
            working_count_.fetch_sub(1);
            const auto end_run = std::chrono::steady_clock::now();
            sum_elapsed_run += std::chrono::duration<double>(end_run - start_run).count();
          } else {
            working_count_.fetch_sub(1);
            // std::this_thread::yield();
          }
        }
        const auto end = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration<double>(end - start).count();

        meta_elapsed_[i] = elapsed;
        meta_sum_elapsed_run_[i] = sum_elapsed_run;

#ifdef DEBUG
        for (int j = 0; j < 5; ++j) {
          DEBUG_ASSERT(stop_flag_);
          DEBUG_ASSERT(tasks_.empty());
          DEBUG_ASSERT_EQ(working_count_, 0);
          std::this_thread::sleep_for(std::chrono::seconds{ 1 });
        }
#endif
      });
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;

  ThreadPool&
  operator=(const ThreadPool&) = delete;
  ThreadPool&
  operator=(ThreadPool&&) = delete;

  void
  stop()
  {
    stop_flag_ = true;
    for (std::thread& worker : workers_)
      if (worker.joinable())
        worker.join();
    DEBUG_ASSERT(stop_flag_);
    DEBUG_ASSERT(tasks_.empty());
    DEBUG_ASSERT_EQ(working_count_, 0);
    for (std::size_t i = 0; i < num_threads; ++i) {
      std::cout << "[Worker " << i << "] ";
      std::cout << "[waiting:" << std::fixed << (meta_elapsed_[i] - meta_sum_elapsed_run_[i]) << "s]";
      std::cout << "[running:" << std::fixed << meta_sum_elapsed_run_[i] << "s]\n";
    }
  }

  ~ThreadPool()
  {
    if (!stop_flag_)
      stop();
  }

  void
  enqueue(std::shared_ptr<Task> task, std::size_t tid)
  {
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());
    DEBUG_ASSERT_BETWEEN(tid, 0, num_threads);
    tasks_.push(std::move(task), tid);
  }

  void
  enqueue_round(std::shared_ptr<Task> task, std::size_t tid)
  {
    tasks_.push(std::move(task), tid % num_threads);
  }

  void
  enqueue(std::shared_ptr<Task> task)
  {
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());
    tasks_.push(std::move(task));
  }

  void
  await(const std::shared_ptr<Task>& task, std::size_t tid)
  {
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT_BETWEEN(tid, 0, num_threads);
    while (!task->finished()) {
      auto optional_task = tasks_.try_pop(tid);
      if (optional_task.has_value()) {
        working_count_.fetch_add(1, std::memory_order_release);
        const auto gap_task = optional_task.value();
        DEBUG_ASSERT_NE(gap_task, nullptr);
        DEBUG_ASSERT(!gap_task->finished());
        gap_task->run(tid);
        working_count_.fetch_sub(1, std::memory_order_release);
      }
    }
  }

  static void
  await(const std::shared_ptr<Task>& task)
  {
    DEBUG_ASSERT_NE(task, nullptr);
    while (!task->finished())
      std::this_thread::yield();
  }

  bool
  stopped()
  {
    return stop_flag_;
  }

  std::size_t
  idle_hint() const
  {
    return num_threads - working_count_;
  }

private:
  std::size_t num_threads;
  std::atomic_bool stop_flag_;

  std::vector<std::thread> workers_;
  std::atomic_int32_t working_count_;

  StealTaskStack tasks_;
  std::mutex tasks_mutex_;

  std::vector<double> meta_sum_elapsed_run_;
  std::vector<double> meta_elapsed_;
};
