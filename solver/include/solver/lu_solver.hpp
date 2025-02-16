#pragma once

#include "../linear_equation.hpp"
#include "./common.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <span>
#include <thread_pool.hpp>
#include <utility>
#include <vector>

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

}

class LUSolver final : public Task
{
private:
  LinearEquation le_;
  std::vector<double> result_;

public:
  explicit LUSolver(LinearEquation le_, std::size_t task_id = 0)
    : Task(task_id)
    , le_(std::move(le_))
  {
  }

  ~LUSolver() override = default;

  void
  operator()(std::size_t tid) override
  {
    result_ = lu_solver::solve(std::move(le_));
  }

  std::vector<double>
  result()
  {
    return std::move(result_);
  }
};
