#pragma once

#include <debug.hpp>
#include <util.hpp>

#include <atomic>
#include <cstddef>

class ThreadPool;

class Task
{
  friend ThreadPool;
  std::atomic_bool finished_;
  std::size_t task_id_;

public:
  explicit Task(std::size_t task_id = 0)
    : finished_(false)
    , task_id_(task_id)
  {
  }

  virtual ~Task() = default;

  virtual void
  operator()(std::size_t tid) = 0;

  void
  run(std::size_t thread_id)
  {
    DEBUG_ASSERT(!finished(), "task already performed. tasks are not reusable!");
    this->operator()(thread_id);
    finished_.store(true, std::memory_order_release);
  }

  bool
  finished() const
  {
    return finished_.load(std::memory_order_acquire);
  }

  std::size_t
  task_id() const
  {
    return task_id_;
  }
};

#define TASK_INIT_PACKED(PACKED_ARG) TAIL(UNPACK(PACKED_ARG))(std::move(TAIL(UNPACK(PACKED_ARG))))

#define TASK(NAME, ...)                                                             \
  using NAME = struct NAME final : Task                                             \
  {                                                                                 \
    JOIN(;, FOR_EACH(JOIN_UNPACK, POP(__VA_ARGS__)));                               \
                                                                                    \
    explicit NAME(FOR_EACH(JOIN_UNPACK, POP(__VA_ARGS__)), std::size_t task_id = 0) \
      : Task(task_id) __VA_OPT__(, ) FOR_EACH(TASK_INIT_PACKED, POP(__VA_ARGS__))   \
    {                                                                               \
    }                                                                               \
                                                                                    \
    ~NAME() override = default;                                                     \
                                                                                    \
    void                                                                            \
    operator()(std::size_t thread_id) override                                      \
    {                                                                               \
      HEAD(REVERSE(__VA_ARGS__));                                                   \
    }                                                                               \
  }
