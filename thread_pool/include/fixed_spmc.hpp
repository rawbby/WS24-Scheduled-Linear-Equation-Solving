#pragma once

#include <assert.hpp>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>

template<typename T>
class FixedSPMC
{
public:
  explicit FixedSPMC(std::size_t capacity)
    : capacity_(std::bit_ceil(capacity))
    , buffer_(std::make_unique<std::shared_ptr<T>[]>(capacity_))
    , dequeue_(0)
    , dequeued_(0)
    , enqueued_(0)
  {
    ASSERT_EQ(capacity_ & (capacity_ - 1), 0, "capacity of SPMC must be a power of two");
  }

  bool
  try_push(const std::shared_ptr<T>& item)
  {
    return try_push_(item);
  }

  bool
  try_push(std::shared_ptr<T>&& item)
  {
    return try_push_(std::move(item));
  }

  std::optional<std::shared_ptr<T>>
  try_pop()
  {
    std::size_t dequeue = dequeue_.load(std::memory_order_relaxed);
    while (dequeue < enqueued_.load(std::memory_order_acquire)) { // while not empty
      if (dequeue_.compare_exchange_weak(dequeue, dequeue + 1, std::memory_order_acquire, std::memory_order_relaxed)) {
        const auto item = std::move(buffer_[dequeue & (capacity_ - 1)]);
        std::size_t expected = dequeue;
        while (!dequeued_.compare_exchange_weak(expected, dequeue + 1, std::memory_order_release, std::memory_order_relaxed))
          expected = dequeue;
        return item;
      }
    }
    return std::nullopt;
  }

  std::size_t
  size() const
  {
    const std::size_t enqueued = enqueued_.load(std::memory_order_acquire);
    const std::size_t dequeued = dequeued_.load(std::memory_order_acquire);
    return enqueued - dequeued;
  }

  std::size_t
  empty() const
  {
    return size() == 0;
  }

private:
  bool
  try_push_(std::shared_ptr<T> item)
  {
    const std::size_t enqueued = enqueued_.load(std::memory_order_relaxed);
    const std::size_t dequeued = dequeued_.load(std::memory_order_acquire);
    if (enqueued - dequeued >= capacity_)
      return false; // full
    buffer_[enqueued & (capacity_ - 1)] = std::move(item);
    enqueued_.store(enqueued + 1, std::memory_order_release);
    return true;
  }

private:
  const std::size_t capacity_;
  std::unique_ptr<std::shared_ptr<T>[]> buffer_;

  alignas(64) std::atomic_size_t dequeue_;
  alignas(64) std::atomic_size_t dequeued_;
  alignas(64) std::atomic_size_t enqueued_;
};
