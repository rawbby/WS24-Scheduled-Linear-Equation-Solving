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
  const LinearEquation instance{ 3, { 2, -1, 0, -1, 2, -1, 0, -1, 2 }, { 1, 0, 1 } };

  const auto x_gauss = gauss_solver::solve(instance);
  const auto x_lu = lu_solver::solve(instance);

  auto pool = ThreadPool{};
  const auto x_lu_parallel = lu_solver::solve_parallel(instance, pool);

  for (std::size_t i = 0; i < instance.n; ++i) {
    ASSERT_NEAR(x_gauss[i], x_lu[i], 1e-9, "Mismatch at index %zu in fixed system", i);
    ASSERT_NEAR(x_gauss[i], x_lu_parallel[i], 1e-9, "Mismatch at index %zu in fixed system", i);
  }
}
