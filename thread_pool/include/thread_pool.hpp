#pragma once

#include "./fixed_spmc.hpp"
#include "./memory.hpp"
#include "./mpmc.hpp"
#include "./task.hpp"

#include <bench.hpp>
#include <debug.hpp>
#include <thread_affinity.hpp>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <ostream>
#include <stack>
#include <thread>
#include <vector>

static std::string thread_dump_suffix_{};

class ThreadPool
{
private:
  const std::size_t num_threads_;
  std::atomic_bool stop_flag_;

  std::vector<std::thread> workers_;
  std::atomic_int32_t working_count_;

  std::vector<std::uint64_t> meta_sum_run_duration_;
  std::vector<std::uint64_t> meta_sum_wait_;

  std::atomic_size_t task_id_counter_;

public:
  explicit ThreadPool(std::size_t num_threads = std::max(1u, std::thread::hardware_concurrency() - 1u))
    : num_threads_(num_threads)
    , stop_flag_(false)
    , working_count_(num_threads)
    , meta_sum_run_duration_(num_threads)
    , meta_sum_wait_(num_threads)
    , task_id_counter_(1)
    , tasks_(num_threads * 0x100)
    , local_tasks_(num_threads, 0x800)
    , overflow_tasks_(num_threads)
    , size_(0)
  {
    for (std::size_t i = 0; i < num_threads; ++i) {
      workers_.emplace_back([this, i] {
        pin_to_core(i);
        auto ofs = std::ofstream{ "t" + std::to_string(i) + "_" + thread_dump_suffix_ + ".dump", std::ios_base::binary };

        std::uint64_t sum_task_duration_ns = 0;
        START(thread_duration);

        std::optional<std::shared_ptr<Task>> task;
        while ((task = pop(i)).has_value()) {

          START(task);
          task.value()->run(i);
          const auto task_duration_ns = END(task);
          REDUCE(thread_duration, task_duration_ns);
          sum_task_duration_ns += task_duration_ns;

          static_assert(std::endian::native == std::endian::little);
          ofs.write(reinterpret_cast<const char*>(&task.value()->task_id_), sizeof(task.value()->task_id_));
          ofs.write(reinterpret_cast<const char*>(&time_point_task_start_), sizeof(time_point_task_start_));
          ofs.write(reinterpret_cast<const char*>(&task_duration_ns), sizeof(task_duration_ns));
        }
        meta_sum_wait_[i] = END(thread_duration);
        meta_sum_run_duration_[i] = sum_task_duration_ns;

#ifdef DEBUG
        DEBUG_ASSERT(stop_flag_);
        DEBUG_ASSERT(empty());
        DEBUG_ASSERT_EQ(working_count_, 0);
#endif
        ofs.close();
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
    push_vc_.notify_all();
    for (std::thread& worker : workers_)
      if (worker.joinable())
        worker.join();
    DEBUG_ASSERT(stop_flag_);
    DEBUG_ASSERT(tasks_.empty());
    DEBUG_ASSERT_EQ(working_count_, 0);
    for (std::size_t i = 0; i < num_threads_; ++i) {
      std::cout << "[Worker " << i << "] ";
      std::cout << "[waiting:" << std::fixed << static_cast<double>(meta_sum_wait_[i] / 1000ull) / 1000.0 << "ms]";
      std::cout << "[running:" << std::fixed << static_cast<double>(meta_sum_run_duration_[i] / 1000ull) / 1000.0 << "ms]\n";
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
    DEBUG_ASSERT_BETWEEN(tid, 0, num_threads_);
    push(std::move(task), tid);
  }

  void
  enqueue_round(std::shared_ptr<Task> task, std::size_t tid)
  {
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());
    push(std::move(task), tid % num_threads_);
  }

  void
  enqueue(std::shared_ptr<Task> task)
  {
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());
    task->task_id_ = task_id_counter_.fetch_add(1);
    push(std::move(task));
  }

  void
  await(const std::shared_ptr<Task>& task, std::size_t tid)
  {
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT_BETWEEN(tid, 0, num_threads_);
    while (!task->finished()) {
      auto optional_task = try_pop(tid);
      if (optional_task.has_value()) {
        const auto& gap_task = optional_task.value();
        DEBUG_ASSERT_NE(gap_task, nullptr);
        DEBUG_ASSERT(!gap_task->finished());
        gap_task->run(tid);
      } else {
        std::this_thread::yield();
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

private:
  std::mutex push_mtx_;
  std::condition_variable push_vc_;

  // goto for producer tasks (lock-free)
  FixedSPMC<Task> tasks_;

  // goto for worker sub-tasks (lock-free)
  dynamic_array<FixedSPMC<Task>> local_tasks_;

  // overflow for producer tasks and worker sub-tasks
  dynamic_array<MPMC<Task>> overflow_tasks_;

  std::atomic_size_t size_;

private:
  void
  push(std::shared_ptr<Task> task, std::size_t tid)
  {
    size_.fetch_add(1, std::memory_order_release);
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());
    DEBUG_ASSERT_LT(tid, num_threads_);
    if (!local_tasks_[tid].try_push(task))
      overflow_tasks_[tid].push(std::move(task));
    push_vc_.notify_one();
  }

  void
  push(std::shared_ptr<Task> task)
  {
    size_.fetch_add(1, std::memory_order_release);
    DEBUG_ASSERT_NE(task, nullptr);
    DEBUG_ASSERT(!task->finished());
    if (tasks_.try_push(task)) {
      push_vc_.notify_one();
      return;
    }
    auto min_i = std::numeric_limits<std::size_t>::min();
    auto min_s = std::numeric_limits<std::size_t>::max();
    for (std::size_t i = 0; i < num_threads_; ++i) {
      const auto s = overflow_tasks_[i].size_hint();
      if (!s) {
        overflow_tasks_[i].push(std::move(task));
        push_vc_.notify_one();
        return;
      }
      if (s < min_s) {
        min_i = i;
        min_s = s;
      }
    }
    overflow_tasks_[min_i].push(std::move(task));
    push_vc_.notify_one();
  }

  std::optional<std::shared_ptr<Task>>
  try_pop(std::size_t tid)
  {
    std::optional<std::shared_ptr<Task>> task;
    const auto get_task = [this, &task] {
      DEBUG_ASSERT_NE(task.value(), nullptr);
      DEBUG_ASSERT(!task.value()->finished());
      size_.fetch_sub(1, std::memory_order_release);
      return task.value();
    };
    const auto set_task = [&task](auto& container) {
      task = container.try_pop();
      return task.has_value();
    };

    if (empty())
      return std::nullopt;

    if (set_task(local_tasks_[tid]))
      return get_task();
    if (set_task(local_tasks_[tid]))
      return get_task();
    if (set_task(overflow_tasks_[tid]))
      return get_task();
    if (set_task(tasks_))
      return get_task();

    for (std::size_t i = tid + 1; i < num_threads_; ++i)
      if (set_task(overflow_tasks_[i]))
        return get_task();
    for (std::size_t i = 0; i < tid; ++i)
      if (set_task(overflow_tasks_[i]))
        return get_task();

    for (std::size_t i = tid + 1; i < num_threads_; ++i)
      if (set_task(local_tasks_[i]))
        return get_task();
    for (std::size_t i = 0; i < tid; ++i)
      if (set_task(local_tasks_[i]))
        return get_task();

    return std::nullopt;
  }

  std::optional<std::shared_ptr<Task>>
  pop(std::size_t tid)
  {
    std::unique_lock lock{ push_mtx_, std::defer_lock };
    for (;;) {
      const auto task = try_pop(tid);
      if (task.has_value())
        return task.value();

      while (empty()) {
        working_count_.fetch_sub(1, std::memory_order::release);
        if (stop_flag_ && !working_count_) {
          push_vc_.notify_all();
          return std::nullopt;
        }
        // wait for stop or work
        lock.lock();
        push_vc_.wait(lock, [this] { return (stop_flag_ && !working_count_) || !empty(); });
        lock.unlock();
        if (stop_flag_ && !working_count_)
          return std::nullopt;
        working_count_.fetch_add(1, std::memory_order::release);
      }
    }
  }

public:
  std::size_t
  num_threads() const
  {
    return num_threads_;
  }
  std::size_t
  idle() const
  {
    return num_threads_ - working_count_.load(std::memory_order_acquire);
  }

  std::size_t
  load() const
  {
    return size_.load(std::memory_order_acquire);
  }

  bool
  empty() const
  {
    return size_.load(std::memory_order_acquire) == 0;
  }
};
