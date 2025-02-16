#include <linear_equation.hpp>
#include <solver.hpp>
#include <thread_pool.hpp>

#include <assert.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

int
main()
{
  constexpr std::size_t n = 512;
  const auto instance = generate_diagonally_dominant_system(n);

  const auto x_gauss = gauss_solver::solve(instance);
  const auto x_lu = lu_solver::solve(instance);
  const auto x_lu_parallel = lu_solver::solve_parallel(instance);

  auto compute_residual = [&](const std::vector<double>& x) -> double {
    double max_residual = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
      double Ax_i = 0.0;
      for (std::size_t j = 0; j < n; ++j)
        Ax_i += instance.A[i * n + j] * x[j];

      const double residual = std::abs(Ax_i - instance.b[i]);
      if (residual > max_residual)
        max_residual = residual;
    }
    return max_residual;
  };

  const double res_gauss = compute_residual(x_gauss);
  const double res_lu = compute_residual(x_lu);
  const double res_lu_parallel = compute_residual(x_lu_parallel);

  ASSERT_LT(res_gauss, 1e-6, "GaussSolver residual too high: %f", res_gauss);
  ASSERT_LT(res_lu, 1e-1, "LUSolver residual too high: %f", res_lu);
  ASSERT_LT(res_lu_parallel, 1e-1, "LUParallelSolver residual too high: %f", res_lu_parallel);
}
