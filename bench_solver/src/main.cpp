#include <linear_equation.hpp>
#include <solver.hpp>
#include <thread_pool.hpp>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>

int
main()
{
  std::srand(static_cast<unsigned int>(std::time(nullptr)));

  constexpr int block_sizes[] = { 16 };
  constexpr int thread_counts[] = { 1, 2 }; // , 3, 4, 6, 8 };
  constexpr std::size_t n = 2048;

  const auto instance = generate_diagonally_dominant_system(n);

  {
    auto pool = std::make_shared<ThreadPool>(1);
    auto task = std::make_shared<lu_solver::LUSolverTask>(instance);
    const auto start = std::chrono::steady_clock::now();
    pool->enqueue(task);
    pool->await(task);
    const auto end = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(end - start).count();
    std::cout << "LUSolverTask took " << elapsed << " seconds." << std::endl;
  }
  {
    auto pool = std::make_shared<ThreadPool>(6);
    auto task = std::make_shared<lu_solver::LUSolverParallelTask>(instance, pool);
    const auto start = std::chrono::steady_clock::now();
    pool->enqueue(task);
    pool->await(task);
    const auto end = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(end - start).count();
    std::cout << "LUSolverParallelTask took " << elapsed << " seconds." << std::endl;
  }

  // Helper lambda to run a solver and report elapsed time.
  // auto benchmark = [&](const std::string& name, auto solver_func) {
  //   const auto start = std::chrono::steady_clock::now();
  //   const auto x = solver_func(instance);
  //   const auto end = std::chrono::steady_clock::now();
  //   const double elapsed = std::chrono::duration<double>(end - start).count();
  //   std::cout << name << " took " << elapsed << " seconds." << std::endl;
  //   return std::pair{ x, elapsed };
  // };

  // double min_single_elapsed;
  // {
  //   const auto [_, gauss_elapsed] = benchmark("GaussSolver", [](const LinearEquation& instance) { return gauss_solver::solve(instance); });
  //   min_single_elapsed = std::min(min_single_elapsed, gauss_elapsed);
  // }
  // {
  //   const auto [_, gauss_elapsed] = benchmark("GaussSolver", [](const LinearEquation& instance) { return gauss_solver::solve(instance); });
  //   min_single_elapsed = std::min(min_single_elapsed, gauss_elapsed);
  // }
  // {
  //   const auto [_, gauss_elapsed] = benchmark("GaussSolver", [](const LinearEquation& instance) { return gauss_solver::solve(instance); });
  //   min_single_elapsed = std::min(min_single_elapsed, gauss_elapsed);
  // }
  // std::cout << std::endl;

  // for (int bs : block_sizes) {
  //   {
  //     const auto name = "LUSolver (" + std::to_string(bs) + " block size)";
  //     const auto [_, lu_elapsed] = benchmark(name, [bs](const LinearEquation& instance) { return lu_solver::solve(instance, static_cast<std::size_t>(bs));
  //     }); min_single_elapsed = std::min(min_single_elapsed, lu_elapsed);
  //   }
  //   {
  //     const auto name = "LUSolver (" + std::to_string(bs) + " block size)";
  //     const auto [_, lu_elapsed] = benchmark(name, [bs](const LinearEquation& instance) { return lu_solver::solve(instance, static_cast<std::size_t>(bs));
  //     }); min_single_elapsed = std::min(min_single_elapsed, lu_elapsed);
  //   }
  //   {
  //     const auto name = "LUSolver (" + std::to_string(bs) + " block size)";
  //     const auto [_, lu_elapsed] = benchmark(name, [bs](const LinearEquation& instance) { return lu_solver::solve(instance, static_cast<std::size_t>(bs));
  //     }); min_single_elapsed = std::min(min_single_elapsed, lu_elapsed);
  //   }
  //   std::cout << std::endl;
  // }

  // for (int bs : block_sizes) {
  //   for (int tc : thread_counts) {
  //     {
  //       const auto name = "LUParallelSolver (" + std::to_string(tc) + " threads, " + std::to_string(bs) + " block size)";
  //       const auto [_, lu_elapsed] = benchmark(name, [bs, tc](const LinearEquation& instance) {
  //         auto pool = ThreadPool{ static_cast<std::size_t>(tc) };
  //         return lu_solver::solve_parallel(instance, pool, static_cast<std::size_t>(bs));
  //       });
  //       std::cout << name << " efficiency " << 100.0 * (min_single_elapsed / (lu_elapsed * static_cast<double>(tc))) << "%" << std::endl << std::endl;
  //     }
  //     {
  //       const auto name = "LUParallelSolver (" + std::to_string(tc) + " threads, " + std::to_string(bs) + " block size)";
  //       const auto [_, lu_elapsed] = benchmark(name, [bs, tc](const LinearEquation& instance) {
  //         auto pool = ThreadPool{ static_cast<std::size_t>(tc) };
  //         return lu_solver::solve_parallel(instance, pool, static_cast<std::size_t>(bs));
  //       });
  //       std::cout << name << " efficiency " << 100.0 * (min_single_elapsed / (lu_elapsed * static_cast<double>(tc))) << "%" << std::endl << std::endl;
  //     }
  //     {
  //       const auto name = "LUParallelSolver (" + std::to_string(tc) + " threads, " + std::to_string(bs) + " block size)";
  //       const auto [_, lu_elapsed] = benchmark(name, [bs, tc](const LinearEquation& instance) {
  //         auto pool = ThreadPool{ static_cast<std::size_t>(tc) };
  //         return lu_solver::solve_parallel(instance, pool, static_cast<std::size_t>(bs));
  //       });
  //       std::cout << name << " efficiency " << 100.0 * (min_single_elapsed / (lu_elapsed * static_cast<double>(tc))) << "%" << std::endl << std::endl;
  //     }
  //   }
  // }
}
