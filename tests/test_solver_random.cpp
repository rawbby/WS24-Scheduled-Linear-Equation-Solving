#include <linear_equation.hpp>
#include <solver.hpp>
#include <thread_pool.hpp>

#include <assert.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>

void
test_small_random_system()
{
  constexpr std::size_t n = 16;
  const auto instance = generate_diagonally_dominant_system(n);

  const auto x_lu = lu_solver::solve(instance);

  auto pool = ThreadPool{};
  const auto x_lu_parallel = lu_solver::solve_parallel(instance, pool, 0);

  for (std::size_t i = 0; i < n; ++i) {
    ASSERT_NEAR(x_lu[i], x_lu_parallel[i], 1e-4, "Mismatch at index %zu in random system (%f !~= %f)", i, x_lu[i], x_lu_parallel[i]);
  }
}

void
test_big_random_system()
{
  constexpr std::size_t n = 1024;

  const auto instance = generate_diagonally_dominant_system(n);

  const auto x_lu = lu_solver::solve(instance);

  auto pool = ThreadPool{};
  const auto x_lu_parallel = lu_solver::solve_parallel(instance, pool, 0);

  for (std::size_t i = 0; i < n; ++i) {
    ASSERT_NEAR(x_lu[i], x_lu_parallel[i], 1e-4, "Mismatch at index %zu in random system (%f !~= %f)", i, x_lu[i], x_lu_parallel[i]);
  }
}

int
main()
{
  std::srand(static_cast<unsigned int>(std::time(nullptr)));
  test_big_random_system();
  test_small_random_system();
}
