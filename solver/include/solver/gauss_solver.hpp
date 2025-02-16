#pragma once

#include "../linear_equation.hpp"
#include "./common.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <span>
#include <thread_pool.hpp>
#include <utility>
#include <vector>

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

class GaussSolver final : public Task
{
private:
  LinearEquation le_;
  std::vector<double> result_;

public:
  explicit GaussSolver(LinearEquation le_, std::size_t task_id = 0)
    : Task(task_id)
    , le_(std::move(le_))
  {
  }

  ~GaussSolver() override = default;

  void
  operator()(std::size_t tid) override
  {
    result_ = gauss_solver::solve(std::move(le_));
  }

  std::vector<double>
  result()
  {
    return std::move(result_);
  }
};
