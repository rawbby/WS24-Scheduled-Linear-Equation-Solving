#pragma once

#include <linear_equation.hpp>
#include <solver.hpp>
#include <thread_pool.hpp>

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

class LinearEquationSchedulerBase
{
public:
  virtual ~LinearEquationSchedulerBase() = default;

  LinearEquationSchedulerBase(std::shared_ptr<std::queue<LinearEquation>> queue, std::shared_ptr<std::mutex> mtx, std::shared_ptr<std::condition_variable> cv, std::size_t num_threads)
    : stop_flag_(false)
    , queue_(std::move(queue))
    , queue_mutex_(std::move(mtx))
    , queue_cv_(std::move(cv))
    , pool_(std::make_shared<ThreadPool>(num_threads))
  {
  }

  void
  start()
  {
    scheduler_thread_ = std::thread(&LinearEquationSchedulerBase::run, this);
  }

  void
  stop()
  {
    std::unique_lock lock(*queue_mutex_);
    stop_flag_ = true;
    lock.unlock();
    queue_cv_->notify_all();
    if (scheduler_thread_.joinable()) {
      scheduler_thread_.join();
    }
    pool_->stop();
  }

private:
  void
  run()
  {
    const auto start = std::chrono::steady_clock::now();
    std::unique_lock lock(*queue_mutex_, std::defer_lock);
    while (true) {
      lock.lock();
      queue_cv_->wait(lock, [&] { return !queue_->empty() || stop_flag_; });
      if (stop_flag_ && queue_->empty())
        break;

      auto linear_equation = std::move(queue_->front());
      queue_->pop();
      auto queued = queue_->size();

      const auto now = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
      std::cout << "[Scheduler] ";
      std::cout << "[t:" << std::fixed << now << "s]";
      std::cout << "[n:" << std::fixed << linear_equation.n << "]";
      std::cout << "[score:" << std::fixed << linear_equation.score << "s]\n";
      lock.unlock();

      on_linear_equation(std::move(linear_equation), queued);
    }
  }

  std::atomic<bool> stop_flag_;

  std::shared_ptr<std::queue<LinearEquation>> queue_;
  std::shared_ptr<std::mutex> queue_mutex_;
  std::shared_ptr<std::condition_variable> queue_cv_;

  std::thread scheduler_thread_;

protected:
  virtual void
  on_linear_equation(LinearEquation&& le, std::size_t queued) = 0;

  std::shared_ptr<ThreadPool> pool_;
};
