#pragma once

#include "../linear_equation.hpp"
#include "./lu_solver.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <thread_pool.hpp>
#include <utility>
#include <vector>

class LUSolverParallel final : public Task
{
private:
  LinearEquation le_;
  std::shared_ptr<ThreadPool> pool_;
  std::vector<double> result_;

public:
  LUSolverParallel(LinearEquation le_, std::shared_ptr<ThreadPool> pool_, std::size_t task_id = 0)
    : Task(task_id)
    , le_(std::move(le_))
    , pool_(std::move(pool_))
  {
  }

  ~LUSolverParallel() override = default;

  void
  operator()(std::size_t tid) override
  {
    using namespace solver::common;
    using namespace lu_solver;

    auto& [n, A, b, _] = le_;

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
      pool_->enqueue_round(u12, tid);
      update_l21(A, n, i, ib);
      pool_->await(u12, tid);

      // update_u12(A, n, i, ib);
      // update_l21(A, n, i, ib);

      update_a22_parallel(A, n, i, ib, *pool_, tid);
    }

    auto y = forward_substitution(A, b, n);
    result_ = back_substitution(A, y, n);
    pool_.reset();
  }

  std::vector<double>
  result()
  {
    return std::move(result_);
  }
};

namespace lu_solver {

inline std::vector<double>
solve_parallel(LinearEquation le, std::size_t num_threads = std::thread::hardware_concurrency())
{
  auto pool = std::make_shared<ThreadPool>(num_threads);
  auto task = std::make_shared<LUSolverParallel>(std::move(le), pool);
  pool->enqueue(task);
  pool->await(task);
  return task->result();
}

}
