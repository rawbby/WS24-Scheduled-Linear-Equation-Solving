#pragma once

#include "./linear_equation_scheduler_base.hpp"

#include <solver.hpp>

class LES_trivial final : public LinearEquationSchedulerBase
{
public:
  ~LES_trivial() override = default;

  LES_trivial(std::shared_ptr<std::queue<LinearEquation>> queue, std::shared_ptr<std::mutex> mtx, std::shared_ptr<std::condition_variable> cv, std::size_t num_threads)
    : LinearEquationSchedulerBase(std::move(queue), std::move(mtx), std::move(cv), num_threads)
  {
  }

protected:
  void
  on_linear_equation(LinearEquation&& le, std::size_t queued) override
  {
    pool_->enqueue(std::make_shared<LUSolver>(std::move(le)));
  }
};
