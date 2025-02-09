#include <linear_equation.hpp>
#include <solver.hpp>
#include <thread_pool.hpp>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>

constexpr std::size_t
n_from_mb(std::size_t mb)
{
  const std::size_t bytes = mb * 1024 * 1024;
  const std::size_t ratio = (bytes + sizeof(double) - 1) / sizeof(double);
  std::size_t low = 0, high = ratio;
  while (low < high) {
    std::size_t mid = low + (high - low) / 2;
    if (mid * mid < ratio)
      low = mid + 1;
    else
      high = mid;
  }
  const std::size_t sqrt_val = low;
  std::size_t power = 1;
  while (power < sqrt_val)
    power *= 2;
  return power;
}

int
main()
{
#ifdef DEBUG
  constexpr std::size_t problem_sizes[] = {
    n_from_mb(2),
    n_from_mb(8),
    n_from_mb(32),
  };
#else
  constexpr std::size_t problem_sizes[] = {
    // n_from_mb(2),
    // n_from_mb(8),
    // n_from_mb(32),
    n_from_mb(128),
    // n_from_mb(512)
  };
#endif

  for (const auto n : problem_sizes) {
    std::cout << std::endl
              << "Problem size " << n << "*" << n << " (" << (n * n * sizeof(double)) / (1024 * 1024) << "MB)" << std::endl;

    const auto filename = std::string("problem") + std::to_string(n) + std::string(".raw");
    if (!std::filesystem::directory_entry{ filename }.exists()) {
      const auto series = generate_diagonally_dominant_system(n);
      series.serialize(filename);
    }
    const auto instance = LinearEquation::deserialize(filename);

    // for (int i = 0; i < 1; ++i) {
    //   const auto pool = std::make_shared<ThreadPool>(1);
    //   auto task = std::make_shared<LUSolver>(instance);
    //   const auto start = std::chrono::steady_clock::now();
    //   pool->enqueue(task);
    //   pool->await(task);
    //   const auto end = std::chrono::steady_clock::now();
    //   const double elapsed = std::chrono::duration<double>(end - start).count();
    //   std::cout << "LUSolver took " << std::fixed << elapsed << " seconds." << std::endl;
    // }
    for (int i = 6; i <= 6 /* std::thread::hardware_concurrency() */; i = ((i + 2) & (~1))) {
      const auto pool = std::make_shared<ThreadPool>(i);
      auto task = std::make_shared<LUSolverParallel>(instance, pool);
      const auto start = std::chrono::steady_clock::now();
      pool->enqueue(task);
      pool->await(task);
      const auto end = std::chrono::steady_clock::now();
      const double elapsed = std::chrono::duration<double>(end - start).count();
      std::cout << "LUSolverParallel (" << i << ") took " << std::fixed << elapsed << " seconds." << std::endl;
    }
  }
}
