#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class Task
{
  std::atomic<bool> finished_{ false };

public:
  virtual ~Task() = default;

  virtual void
  operator()() = 0;

  void
  run()
  {
    this->operator()();
    finished_.store(true, std::memory_order_release);
  }

  bool
  finished() const
  {
    return finished_.load(std::memory_order_acquire);
  }
};

class ThreadPool
{
public:
  explicit ThreadPool(std::size_t num_threads = std::thread::hardware_concurrency())
    : stop_flag_(false)
    , working_count_(0)
  {
    for (std::size_t i = 0; i < num_threads; ++i) {
      workers_.emplace_back([this] {
        auto lock = std::unique_lock{ tasks_mutex_ };
        while (true) {
          // wait for stop or a task
          tasks_cv_.wait(lock, [this] { return (stop_flag_ && !working_count_) || !tasks_.empty(); });

          // handle stop
          if (stop_flag_ && !working_count_ && tasks_.empty()) {
            lock.unlock();
            return;
          }

          // handle task
          if (!tasks_.empty()) {
            auto f = tasks_.front();
            tasks_.pop_front();

            working_count_.fetch_add(1, std::memory_order_release);
            lock.unlock();
            f->run();
            if (1 == working_count_.fetch_sub(1, std::memory_order_acq_rel))
              tasks_cv_.notify_all(); // now working count == 0

            lock.lock();
          }
        }
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
    tasks_cv_.notify_all();
    for (std::thread& worker : workers_)
      if (worker.joinable())
        worker.join();
  }

  ~ThreadPool() { stop(); }

  void
  enqueue(std::shared_ptr<Task> task)
  {
    auto lock = std::unique_lock{ tasks_mutex_ };
    tasks_.emplace_back(std::move(task));
    lock.unlock();
    tasks_cv_.notify_one();
  }

  void
  await(const std::shared_ptr<Task>& task)
  {
    auto lock = std::unique_lock{ tasks_mutex_, std::defer_lock };

    // until future is ready do tasks or yield
    while (!task->finished()) {
      lock.lock();
      if (!tasks_.empty()) {
        auto gap_task = tasks_.front();
        tasks_.pop_front();
        lock.unlock();
        gap_task->run();
      } else {
        lock.unlock();
        std::this_thread::yield();
      }
    }
  }

private:
  std::atomic<bool> stop_flag_;

  std::vector<std::thread> workers_;
  std::atomic<int> working_count_;

  std::deque<std::shared_ptr<Task>> tasks_;
  std::mutex tasks_mutex_;
  std::condition_variable tasks_cv_;
};
