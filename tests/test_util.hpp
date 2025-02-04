#pragma once

#include <cstdio>
#include <cstdlib>
#include <type_traits>

#if !defined(NDEBUG)
#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#else
#include <csignal>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif
#else
#define DEBUG_BREAK() ((void)0)
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define ASSERT(cond, ...)                                                                                                                                      \
  do {                                                                                                                                                         \
    if (!(cond)) {                                                                                                                                             \
      std::fprintf(stderr,                                                                                                                                     \
                   "[ASSERTION FAILED]\n"                                                                                                                      \
                   "  Condition  : " #cond "\n"                                                                                                                \
                   "  Location   : " __FILE__ ":" TOSTRING(__LINE__) "\n");                                                                                    \
      __VA_OPT__(std::fprintf(stderr, "  Details   : " __VA_ARGS__); std::fprintf(stderr, "\n");)                                                              \
      DEBUG_BREAK();                                                                                                                                           \
      std::abort();                                                                                                                                            \
    }                                                                                                                                                          \
  } while (false)

#define OPERATOR_ASSERT_FAIL_(op, lhs, rhs, ...)                                                                                                               \
  do {                                                                                                                                                         \
    auto _lVal = (lhs);                                                                                                                                        \
    auto _rVal = (rhs);                                                                                                                                        \
    if constexpr (std::is_floating_point_v<decltype(_lVal)> && std::is_floating_point_v<decltype(_rVal)>) {                                                    \
      std::fprintf(stderr,                                                                                                                                     \
                   "[ASSERTION FAILED]\n"                                                                                                                      \
                   "  Condition  : " #lhs " " op " " #rhs "\n"                                                                                                 \
                   "  Evaluation : %f " op " %f\n"                                                                                                             \
                   "  Location   : " __FILE__ ":" TOSTRING(__LINE__) "\n",                                                                                     \
                   static_cast<double>(_lVal),                                                                                                                 \
                   static_cast<double>(_rVal));                                                                                                                \
    } else if constexpr (std::is_arithmetic_v<decltype(_lVal)> && std::is_arithmetic_v<decltype(_rVal)>) {                                                     \
      std::fprintf(stderr,                                                                                                                                     \
                   "[ASSERTION FAILED]\n"                                                                                                                      \
                   "  Condition  : " #lhs " " op " " #rhs "\n"                                                                                                 \
                   "  Evaluation : %lld " op " %lld\n"                                                                                                         \
                   "  Location   : " __FILE__ ":" TOSTRING(__LINE__) "\n",                                                                                     \
                   static_cast<long long>(_lVal),                                                                                                              \
                   static_cast<long long>(_rVal));                                                                                                             \
    } else {                                                                                                                                                   \
      std::fprintf(stderr,                                                                                                                                     \
                   "[ASSERTION FAILED]\n"                                                                                                                      \
                   "  Condition  : " #lhs " " op " " #rhs "\n"                                                                                                 \
                   "  Location   : " __FILE__ ":" TOSTRING(__LINE__) "\n");                                                                                    \
    }                                                                                                                                                          \
    __VA_OPT__(std::fprintf(stderr, "  Details   : " __VA_ARGS__); std::fprintf(stderr, "\n");)                                                                \
    DEBUG_BREAK();                                                                                                                                             \
    std::abort();                                                                                                                                              \
  } while (false)

#define ASSERT_EQ(lhs, rhs, ...)                                                                                                                               \
  do {                                                                                                                                                         \
    if (!((lhs) == (rhs))) {                                                                                                                                   \
      OPERATOR_ASSERT_FAIL_("==", lhs, rhs, __VA_ARGS__);                                                                                                      \
    }                                                                                                                                                          \
  } while (false)

#define ASSERT_NE(lhs, rhs, ...)                                                                                                                               \
  do {                                                                                                                                                         \
    if (!((lhs) != (rhs))) {                                                                                                                                   \
      OPERATOR_ASSERT_FAIL_("!=", lhs, rhs, __VA_ARGS__);                                                                                                      \
    }                                                                                                                                                          \
  } while (false)

#define ASSERT_LT(lhs, rhs, ...)                                                                                                                               \
  do {                                                                                                                                                         \
    if (!((lhs) < (rhs))) {                                                                                                                                    \
      OPERATOR_ASSERT_FAIL_("<", lhs, rhs, __VA_ARGS__);                                                                                                       \
    }                                                                                                                                                          \
  } while (false)

#define ASSERT_LE(lhs, rhs, ...)                                                                                                                               \
  do {                                                                                                                                                         \
    if (!((lhs) <= (rhs))) {                                                                                                                                   \
      OPERATOR_ASSERT_FAIL_("<=", lhs, rhs, __VA_ARGS__);                                                                                                      \
    }                                                                                                                                                          \
  } while (false)

#define ASSERT_GT(lhs, rhs, ...)                                                                                                                               \
  do {                                                                                                                                                         \
    if (!((lhs) > (rhs))) {                                                                                                                                    \
      OPERATOR_ASSERT_FAIL_(">", lhs, rhs, __VA_ARGS__);                                                                                                       \
    }                                                                                                                                                          \
  } while (false)

#define ASSERT_GE(lhs, rhs, ...)                                                                                                                               \
  do {                                                                                                                                                         \
    if (!((lhs) >= (rhs))) {                                                                                                                                   \
      OPERATOR_ASSERT_FAIL_(">=", lhs, rhs, __VA_ARGS__);                                                                                                      \
    }                                                                                                                                                          \
  } while (false)

#define ASSERT_NEAR(lhs, rhs, tolerance, ...)                                                                                                                  \
  do {                                                                                                                                                         \
    ASSERT_LE((lhs), (rhs) + std::abs((rhs) * (tolerance)), __VA_ARGS__);                                                                                      \
    ASSERT_GE((lhs) + std::abs((lhs) * (tolerance)), (rhs), __VA_ARGS__);                                                                                      \
  } while (false)
