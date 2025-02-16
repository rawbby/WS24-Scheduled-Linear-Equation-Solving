#pragma once

#include "./linear_equation_scheduler_base.hpp"

#include <solver.hpp>

class LES_mixed final : public LinearEquationSchedulerBase
{
  int i = 0;

public:
  ~LES_mixed() override = default;

  LES_mixed(std::shared_ptr<std::queue<LinearEquation>> queue, std::shared_ptr<std::mutex> mtx, std::shared_ptr<std::condition_variable> cv, std::size_t num_threads)
    : LinearEquationSchedulerBase(std::move(queue), std::move(mtx), std::move(cv), num_threads)
  {
  }

protected:
  void
  on_linear_equation(LinearEquation&& le, std::size_t queued) override
  {
    if ((pool_->idle() - 1) <= queued)
      pool_->enqueue_round(std::make_shared<LUSolver>(std::move(le)), ++i);
    else
      pool_->enqueue_round(std::make_shared<LUSolverParallel>(std::move(le), pool_), ++i);
  }
};
