#pragma once

#include "./linear_equation.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
#include <thread_pool.hpp>
#include <utility>
#include <vector>

namespace solver::common {
constexpr auto EPSILON = 1e-12;

/**
 * This function searches the pivot column (starting at index `col`) for the element with the largest
 * absolute value. If a row with a larger pivot element is found, the function swaps that row with
 * the current pivot row in both the matrix and the right-hand side vector.
 */
inline void
partial_pivot(std::span<double> U, std::span<double> b, std::size_t n, std::size_t col)
{
  std::size_t pivot = col;
  double max_val = std::abs(U[(col * n) + col]);

  for (std::size_t row = col + 1; row < n; ++row) {
    const double val = std::abs(U[(row * n) + col]);
    if (val > max_val) {
      max_val = val;
      pivot = row;
    }
  }

  if (max_val < EPSILON)
    throw std::runtime_error("singular matrix");

  if (pivot != col) {
    for (std::size_t j = col; j < n; ++j)
      std::swap(U[(col * n) + j], U[(pivot * n) + j]);
    std::swap(b[col], b[pivot]);
  }
}

/**
 * For the current pivot row `k` (within the panel starting at column `i` and of width `ib`),
 * this function computes the multiplier for each row below (from k+1 to n) by dividing the element
 * A[row * n + k] by the pivot A[k * n + k]. It then updates the entries in the panel columns
 * (from k+1 to i+ib) by subtracting the product of the multiplier and the corresponding pivot row's entry.
 */
inline void
compute_multipliers_and_update_panel(std::span<double> A, std::size_t n, std::size_t k, std::size_t i, std::size_t ib)
{
  for (std::size_t row = k + 1; row < n; ++row) {
    A[(row * n) + k] /= A[(k * n) + k];
    for (std::size_t col = k + 1; col < i + ib; ++col) {
      A[(row * n) + col] -= A[(row * n) + k] * A[(k * n) + col];
    }
  }
}

/**
 * U12 corresponds to the sub-matrix for rows in the current panel ([i, i+ib))
 * and columns to the right of the panel ([i+ib, n)). For each element in U12,
 * this function subtracts the contributions from the already computed L and U factors.
 */
inline void
update_u12(std::span<double> A, std::size_t n, std::size_t i, std::size_t ib)
{
  for (std::size_t r = i; r < i + ib; ++r) {
    for (std::size_t c = i + ib; c < n; ++c) {
      double sum = 0.0;
      for (std::size_t k = i; k < r; ++k)
        sum += A[(r * n) + k] * A[(k * n) + c];
      A[(r * n) + c] -= sum;
    }
  }
}
TASK(U12, (std::span<double>, A), (std::size_t, n), (std::size_t, i), (std::size_t, ib), update_u12(A, n, i, ib));

/**
 * L21 corresponds to the sub-matrix for rows below the current panel ([i+ib, n))
 * and columns within the panel ([i, i+ib)). For each element in L21, the function
 * subtracts the contributions from previously computed multipliers and divides
 * by the corresponding pivot element from the U factor.
 */
inline void
update_l21(std::span<double> A, std::size_t n, std::size_t i, std::size_t ib)
{
  for (std::size_t r = i + ib; r < n; ++r) {
    for (std::size_t c = i; c < i + ib; ++c) {
      double sum = 0.0;
      for (std::size_t k = i; k < c; ++k)
        sum += A[(r * n) + k] * A[(k * n) + c];
      A[(r * n) + c] = (A[(r * n) + c] - sum) / A[(c * n) + c];
    }
  }
}
TASK(L21, (std::span<double>, A), (std::size_t, n), (std::size_t, i), (std::size_t, ib), update_l21(A, n, i, ib));

/**
 * A22 is the submatrix of A starting at row and column index (i+ib) to n.
 * This function updates A22 by subtracting the product of the L21 and U12 blocks:
 *     A22 = A22 - (L21 * U12)
 * The update is done in blocks to improve cache performance.
 */
inline void
update_a22(std::span<double> A, std::size_t n, std::size_t i, std::size_t ib)
{
  if (i + ib < n) {
    for (std::size_t r = i + ib; r < n; ++r) {
      for (std::size_t k = i; k < i + ib; ++k) {
        const double rnk = A[(r * n) + k];
        for (std::size_t c = i + ib; c < n; ++c)
          A[(r * n) + c] -= rnk * A[(k * n) + c];
      }
    }
  }
}

/**
 * This function partitions the rows of A22 into blocks and uses the provided ThreadPool
 * to update each block in parallel. For each block, it subtracts the product of the L21 and U12 blocks:
 *     A22 = A22 - (L21 * U12)
 */
inline void
update_a22_parallel(std::span<double> A, std::size_t n, std::size_t i, std::size_t ib, ThreadPool& pool, std::size_t tid)
{
  TASK(A22, (std::size_t, r), (std::size_t, i), (std::size_t, ib), (std::span<double>, A), (std::size_t, n), {
    for (std::size_t k = i; k < i + ib; ++k) {
      const double rnk = A[r * n + k];
      for (std::size_t c = i + ib; c < n; ++c)
        A[r * n + c] -= rnk * A[k * n + c];
    }
  });

  if (i + ib < n) {
    auto tasks = std::vector<std::shared_ptr<A22>>{};
    for (std::size_t r = i + ib; r < n; ++r)
      pool.enqueue_round(tasks.emplace_back(std::make_shared<A22>(r, i, ib, A, n)), tid + r);

    for (const auto& task : std::views::reverse(tasks))
      pool.await(task, tid);
  }
}

/**
 * Solves the linear system Ux=b using back substitution
 * where U is an upper triangular matrix stored in row-major order
 */
inline std::vector<double>
back_substitution(std::span<double> U, std::span<double> b, std::size_t n)
{
  auto x = std::vector(n, 0.0);
  for (std::size_t i = n; i-- > 0;) {
    double sum = b[i];
    for (std::size_t j = i + 1; j < n; ++j)
      sum -= U[(i * n) + j] * x[j];
    if (-EPSILON < U[(i * n) + i] && U[(i * n) + i] < EPSILON)
      throw std::runtime_error("singular matrix");
    x[i] = sum / U[(i * n) + i];
  }
  return x;
}

/**
 * Solves the lower triangular system Ly=b using forward substitution
 * where L is a lower triangular matrix stored in row-major order and ones on diagonal
 */
inline std::vector<double>
forward_substitution(std::span<double> L, std::span<double> b, std::size_t n)
{
  auto y = std::vector(n, 0.0);
  for (std::size_t i = 0; i < n; ++i) {
    double sum = b[i];
    for (std::size_t j = 0; j < i; ++j)
      sum -= L[(i * n) + j] * y[j];
    y[i] = sum;
  }
  return y;
}
}

namespace gauss_solver {
using namespace solver::common;

inline std::vector<double>
solve(LinearEquation le)
{
  auto& [n, A, b, _] = le;

  // Forward elimination with partial pivoting.
  for (std::size_t i = 0; i < n; ++i) {
    partial_pivot(A, b, n, i);

    // Eliminate below pivot.
    for (std::size_t row = i + 1; row < n; ++row) {
      const double factor = A[(row * n) + i] / A[(i * n) + i];
      b[row] -= factor * b[i];
      for (std::size_t col = i; col < n; ++col) {
        A[(row * n) + col] -= factor * A[(i * n) + col];
      }
    }
  }

  return back_substitution(A, b, n);
}
}

namespace lu_solver {
using namespace solver::common;
constexpr auto BLOCK_SIZE = std::size_t{ 4 };

inline std::vector<double>
solve(LinearEquation le)
{
  auto& [n, A, b, _] = le;

  // blocked lu factorization with partial pivoting
  for (std::size_t i = 0; i < n; i += BLOCK_SIZE) {
    const auto ib = std::min(BLOCK_SIZE, n - i);

    // factorize panel A[i:n][i:i+ib]
    for (std::size_t k = i; k < i + ib; ++k) {
      partial_pivot(A, b, n, k);
      compute_multipliers_and_update_panel(A, n, k, i, ib);
    }

    update_u12(A, n, i, ib);
    update_l21(A, n, i, ib);
    update_a22(A, n, i, ib);
  }

  auto y = forward_substitution(A, b, n);
  return back_substitution(A, y, n);
}

inline std::vector<double>
solve_parallel(LinearEquation le, ThreadPool& pool, std::size_t tid = std::numeric_limits<std::size_t>::max())
{
  auto& [n, A, b, _] = le;

  // blocked lu factorization with partial pivoting
  for (std::size_t i = 0; i < n; i += BLOCK_SIZE) {
    const auto ib = std::min(BLOCK_SIZE, n - i);

    // factorize panel A[i:n][i:i+ib]
    for (std::size_t k = i; k < i + ib; ++k) {
      partial_pivot(A, b, n, k);
      compute_multipliers_and_update_panel(A, n, k, i, ib);
    }

    // move u12 update to another thread
    auto u12 = std::make_shared<U12>(A, n, i, ib);
    pool.enqueue_round(u12, tid + 1);
    update_l21(A, n, i, ib);
    pool.await(u12, tid);

    // update_u12(A, n, i, ib);
    // update_l21(A, n, i, ib);

    update_a22_parallel(A, n, i, ib, pool, tid);
  }

  auto y = forward_substitution(A, b, n);
  return back_substitution(A, y, n);
}

}

TASK(GaussSolver,
     (LinearEquation, le_),
     {
       gauss_solver::solve(std::move(le_));
     });

TASK(LUSolver,
     (LinearEquation, le_),
     {
       lu_solver::solve(std::move(le_));
     });

TASK(LUSolverParallel,
     (LinearEquation, le_),
     (std::shared_ptr<ThreadPool>, pool_),
     {
       lu_solver::solve_parallel(std::move(le_), *pool_, tid);
       pool_.reset();
     });
