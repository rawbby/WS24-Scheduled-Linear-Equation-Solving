#pragma once

#include <linear_equation.hpp>
#include <solver.hpp>
#include <thread_pool.hpp>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

class LinearEquationScheduler
{
public:
  LinearEquationScheduler(std::shared_ptr<std::queue<LinearEquation>> queue, std::shared_ptr<std::mutex> mtx, std::shared_ptr<std::condition_variable> cv)
    : stop_flag_(false)
    , queue_(std::move(queue))
    , queue_mutex_(std::move(mtx))
    , queue_cv_(std::move(cv))
  {
  }

  void
  start()
  {
    scheduler_thread_ = std::thread(&LinearEquationScheduler::run, this);
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
    std::unique_lock lock(*queue_mutex_, std::defer_lock);
    while (true) {
      lock.lock();
      queue_cv_->wait(lock, [&] { return !queue_->empty() || stop_flag_; });
      if (stop_flag_ && queue_->empty()) {
        break;
      }

      auto linear_equation = std::move(queue_->front());
      queue_->pop();
      lock.unlock();

      auto task = std::make_shared<lu_solver::LUSolverParallelTask>(std::move(linear_equation), pool_);
      pool_->enqueue(task);

      // pool_.enqueue(0, [linear_equation] {
      //   auto x = LUSolver::solve(*linear_equation);
      //   return x;
      // });
    }
  }

  std::atomic<bool> stop_flag_;

  std::shared_ptr<std::queue<LinearEquation>> queue_;
  std::shared_ptr<std::mutex> queue_mutex_;
  std::shared_ptr<std::condition_variable> queue_cv_;

  std::shared_ptr<ThreadPool> pool_;
  std::thread scheduler_thread_;
};
