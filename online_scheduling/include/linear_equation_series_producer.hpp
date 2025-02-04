#pragma once

#include <ios>
#include <linear_equation.hpp>

#include <array>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

class LinearEquationSeriesProducer
{
public:
  LinearEquationSeriesProducer(std::string filename,
                               std::shared_ptr<std::queue<LinearEquation>> queue,
                               std::shared_ptr<std::mutex> mtx,
                               std::shared_ptr<std::condition_variable> cv)
    : filename_(std::move(filename))
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
    if (producer_thread_.joinable()) {
      producer_thread_.join();
    }
  }

private:
  void
  run() const
  {
    std::ifstream ifs(filename_, std::ios::binary);
    if (!ifs)
      throw std::runtime_error("failed to open file");

    std::size_t count = 0;
    {
      std::array<char, sizeof(count)> buffer{};
      ifs.read(buffer.data(), buffer.size());
      std::memcpy(&count, buffer.data(), buffer.size());
    }

    for (std::size_t i = 0; i < count; ++i) {
      LinearEquation inst;
      {
        std::array<char, sizeof(inst.n)> buffer{};
        ifs.read(buffer.data(), buffer.size());
        std::memcpy(&inst.n, buffer.data(), buffer.size());
      }
      {
        std::array<char, sizeof(inst.score)> buffer{};
        ifs.read(buffer.data(), buffer.size());
        std::memcpy(&inst.score, buffer.data(), buffer.size());
      }
      inst.A.resize(inst.n * inst.n);
      inst.b.resize(inst.n);
      {
        std::vector<char> buffer(inst.A.size() * sizeof(double));
        ifs.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        std::memcpy(inst.A.data(), buffer.data(), buffer.size());
      }
      {
        std::vector<char> buffer(inst.b.size() * sizeof(double));
        ifs.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        std::memcpy(inst.b.data(), buffer.data(), buffer.size());
      }
      {
        std::unique_lock const lock(*mtx_);
        queue_->push(std::move(inst));
      }
      cv_->notify_one();
    }
    ifs.close();
  }

  std::string filename_;
  std::shared_ptr<std::queue<LinearEquation>> queue_;
  std::shared_ptr<std::mutex> mtx_;
  std::shared_ptr<std::condition_variable> cv_;
  std::thread producer_thread_;
};
