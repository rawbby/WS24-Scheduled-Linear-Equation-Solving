#pragma once

#include <assert.hpp>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>

template<typename T>
class FixedMPSC
{
public:
  explicit FixedMPSC(std::size_t capacity)
    : capacity_(std::bit_ceil(capacity))
    , buffer_(std::make_unique<std::shared_ptr<T>[]>(capacity_))
    , dequeued_(0)
    , enqueued_(0)
    , enqueue_(0)
  {
    ASSERT_EQ(capacity_ & (capacity_ - 1), 0, "capacity of MPSC must be a power of two");
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
    const auto dequeued = dequeued_.load(std::memory_order_relaxed);
    const auto enqueued = enqueue_.load(std::memory_order_acquire);
    if (enqueued == dequeued)
      return std::nullopt;
    const auto item = std::move(buffer_[dequeued & (capacity_ - 1)]);
    dequeued_.store(dequeued + 1, std::memory_order_release);
    return item;
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
  try_push_(const std::shared_ptr<T>& item)
  {
    auto enqueue = enqueue_.load(std::memory_order_relaxed);
    while (enqueue - dequeued_.load(std::memory_order_acquire) < capacity_) {
      if (enqueue_.compare_exchange_weak(enqueue, enqueue + 1, std::memory_order_acquire, std::memory_order_relaxed)) {
        buffer_[enqueue & (capacity_ - 1)] = item;
        auto expected = enqueue;
        while (!enqueued_.compare_exchange_weak(expected, enqueue + 1, std::memory_order_release, std::memory_order_relaxed))
          expected = enqueue;
        return true;
      }
    }
    return false;
  }

private:
  const std::size_t capacity_;
  std::unique_ptr<std::shared_ptr<T>[]> buffer_;

  alignas(64) std::atomic_size_t dequeued_;
  alignas(64) std::atomic_size_t enqueued_;
  alignas(64) std::atomic_size_t enqueue_;
};
