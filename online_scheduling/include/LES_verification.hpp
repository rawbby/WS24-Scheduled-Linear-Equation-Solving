#pragma once

#include "./linear_equation_scheduler_base.hpp"

#include <solver.hpp>

class LES_verification_a final : public LinearEquationSchedulerBase
{
  std::size_t success_count_;
  std::size_t failure_count_;

public:
  ~LES_verification_a() override = default;

  LES_verification_a(std::shared_ptr<std::queue<LinearEquation>> queue, std::shared_ptr<std::mutex> mtx, std::shared_ptr<std::condition_variable> cv)
    : LinearEquationSchedulerBase(std::move(queue), std::move(mtx), std::move(cv), 0)
    , success_count_(0)
    , failure_count_(0)
  {
  }

protected:
  void
  on_linear_equation(LinearEquation&& le, std::size_t queued) override
  {
    const double score = le.score;

    const auto start = std::chrono::steady_clock::now();
    lu_solver::solve(std::move(le));
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration<double>(end - start).count();

    const bool success = (score * 1.25) > elapsed && elapsed > (score * 0.8);

    success_count_ += success;
    failure_count_ += (1 - success);

    std::cout << "[Verification] ";
    std::cout << "[score:" << std::fixed << score << "s]";
    std::cout << "[duration:" << std::fixed << elapsed << "s]";
    std::cout << (success ? "[success]\n" : "[failed]\n");
  }

public:
  double
  success_percent() const
  {
    if (!(success_count_ + failure_count_))
      return std::numeric_limits<double>::quiet_NaN();
    return static_cast<double>(success_count_) * 100.0 / static_cast<double>(success_count_ + failure_count_);
  }
};
class LES_verification_b final : public LinearEquationSchedulerBase
{
  std::size_t success_count_;
  std::size_t failure_count_;

public:
  ~LES_verification_b() override = default;

  LES_verification_b(std::shared_ptr<std::queue<LinearEquation>> queue, std::shared_ptr<std::mutex> mtx, std::shared_ptr<std::condition_variable> cv)
    : LinearEquationSchedulerBase(std::move(queue), std::move(mtx), std::move(cv), 0)
    , success_count_(0)
    , failure_count_(0)
  {
  }

protected:
  void
  on_linear_equation(LinearEquation&& le, std::size_t queued) override
  {
    const double score = le.score;

    const auto start = std::chrono::steady_clock::now();
    auto solver = LUSolver{ std::move(le) };
    solver.run(0);
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration<double>(end - start).count();

    const bool success = (score * 1.25) > elapsed && elapsed > (score * 0.8);

    success_count_ += success;
    failure_count_ += (1 - success);

    std::cout << "[Verification] ";
    std::cout << "[score:" << std::fixed << score << "s]";
    std::cout << "[duration:" << std::fixed << elapsed << "s]";
    std::cout << (success ? "[success]\n" : "[failed]\n");
  }

public:
  double
  success_percent() const
  {
    if (!(success_count_ + failure_count_))
      return std::numeric_limits<double>::quiet_NaN();
    return static_cast<double>(success_count_) * 100.0 / static_cast<double>(success_count_ + failure_count_);
  }
};
