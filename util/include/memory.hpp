#pragma once

#include <memory>
#include <new>
#include <utility>

template<typename T>
class dynamic_array
{
  T* data_;
  std::size_t size_;

public:
  template<typename... Args>
  explicit dynamic_array(std::size_t size, Args&&... args)
    : size_(size)
  {
    data_ = static_cast<T*>(std::malloc(size * sizeof(T)));
    for (std::size_t i = 0; i < size; ++i)
      new (data_ + i) T(std::forward<Args>(args)...);
  }

  // clang-format off
  const T& operator[](std::size_t i) const { return data_[i]; }
        T& operator[](std::size_t i)       { return data_[i]; }
  std::size_t size() const { return size_; }
  // clang-format on
};
