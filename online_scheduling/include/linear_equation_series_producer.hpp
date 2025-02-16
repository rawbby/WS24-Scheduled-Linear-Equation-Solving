#pragma once

#include <linear_equation.hpp>
#include <linear_equation_series.hpp>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <ios>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

class LinearEquationSeriesProducer
{
public:
  LinearEquationSeriesProducer(LinearEquationSeries les, double load_factor, std::shared_ptr<std::queue<LinearEquation>> queue, std::shared_ptr<std::mutex> mtx, std::shared_ptr<std::condition_variable> cv)
    : les_(std::move(les))
    , load_factor(load_factor)
    , queue_(std::move(queue))
    , mtx_(std::move(mtx))
    , cv_(std::move(cv))
  {
  }

  void
  start()
  {
    producer_thread_ = std::thread(&LinearEquationSeriesProducer::run, this);
  }

  void
  join()
  {
    if (producer_thread_.joinable())
      producer_thread_.join();
  }

private:
  void
  run()
  {
    double total_wait = 0.0;
    double total_elapsed = 0.0;
    while (!les_.instances.empty()) {
      const auto start = std::chrono::steady_clock::now();

      const auto le = std::move(les_.instances.back());
      les_.instances.pop_back();

      // it takes score / load factor time to create the next problem

      total_wait += (le.score / load_factor);
      const auto wait = total_wait - total_elapsed;
      std::this_thread::sleep_for(std::chrono::milliseconds{ static_cast<std::size_t>(wait * 1000.0) });

      {
        std::unique_lock const lock(*mtx_);
        queue_->push(le);
      }
      cv_->notify_one();

      const auto end = std::chrono::steady_clock::now();
      const auto elapsed = std::chrono::duration<double>(end - start).count();
      total_elapsed += elapsed;
    }
    {
      std::unique_lock const lock(*mtx_);
      std::cout << "[Producer] ";
      std::cout << "[elapsed:" << std::fixed << total_elapsed << "s]";
      std::cout << "[score/load:" << std::fixed << total_wait << "s]\n";
    }
  }

  LinearEquationSeries les_;
  double load_factor;
  std::shared_ptr<std::queue<LinearEquation>> queue_;
  std::shared_ptr<std::mutex> mtx_;
  std::shared_ptr<std::condition_variable> cv_;
  std::thread producer_thread_;
};
