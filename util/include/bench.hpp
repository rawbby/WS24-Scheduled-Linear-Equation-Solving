#pragma once

#define NOW() static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())

#define START(suffix) auto time_point_##suffix##_start_ = NOW()

#define END(suffix) [time_point_##suffix##_start_] {                   \
  const auto time_point_##suffix##_end_ = NOW();                       \
  ASSERT_LE(time_point_##suffix##_start_, time_point_##suffix##_end_); \
  return time_point_##suffix##_end_ - time_point_##suffix##_start_;    \
}()

#define REDUCE(suffix, duration) [&time_point_##suffix##_start_, duration] { \
  time_point_##suffix##_start_ += duration;                                  \
}()
