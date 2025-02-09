#pragma once

#include "assert.hpp"

#if DEBUG
#define DEBUG_ASSERT(cond, ...) ASSERT(cond, __VA_ARGS__)
#define DEBUG_ASSERT_EQ(lhs, rhs, ...) ASSERT_EQ(lhs, rhs, __VA_ARGS__)
#define DEBUG_ASSERT_NE(lhs, rhs, ...) ASSERT_NE(lhs, rhs, __VA_ARGS__)
#define DEBUG_ASSERT_LT(lhs, rhs, ...) ASSERT_LT(lhs, rhs, __VA_ARGS__)
#define DEBUG_ASSERT_LE(lhs, rhs, ...) ASSERT_LE(lhs, rhs, __VA_ARGS__)
#define DEBUG_ASSERT_GT(lhs, rhs, ...) ASSERT_GT(lhs, rhs, __VA_ARGS__)
#define DEBUG_ASSERT_GE(lhs, rhs, ...) ASSERT_GE(lhs, rhs, __VA_ARGS__)
#define DEBUG_ASSERT_BETWEEN(val, lo, hi, ...) ASSERT_BETWEEN(val, lo, hi, __VA_ARGS__)
#define DEBUG_ASSERT_NEAR(lhs, rhs, tolerance, ...) ASSERT_NEAR(lhs, rhs, tolerance, __VA_ARGS__)
#else
#define DEBUG_ASSERT(cond, ...) ((void)0)
#define DEBUG_ASSERT_EQ(lhs, rhs, ...) ((void)0)
#define DEBUG_ASSERT_NE(lhs, rhs, ...) ((void)0)
#define DEBUG_ASSERT_LT(lhs, rhs, ...) ((void)0)
#define DEBUG_ASSERT_LE(lhs, rhs, ...) ((void)0)
#define DEBUG_ASSERT_GT(lhs, rhs, ...) ((void)0)
#define DEBUG_ASSERT_GE(lhs, rhs, ...) ((void)0)
#define DEBUG_ASSERT_BETWEEN(val, lo, hi, ...) ((void)0)
#define DEBUG_ASSERT_NEAR(lhs, rhs, tolerance, ...) ((void)0)
#endif
