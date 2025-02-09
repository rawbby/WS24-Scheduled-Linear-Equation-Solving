#include <les_mixed.hpp>
#include <les_parallel.hpp>
#include <les_trivial.hpp>
#include <linear_equation.hpp>
#include <linear_equation_series.hpp>
#include <linear_equation_series_producer.hpp>

#include <chrono>
#include <filesystem>
#include <iostream>

int
main()
{
  const auto problem_queue = std::make_shared<std::queue<LinearEquation>>();
  const auto queue_mutex = std::make_shared<std::mutex>();
  const auto queue_cv = std::make_shared<std::condition_variable>();

  double total_time = 0.0;

#ifdef DEBUG
  constexpr auto min_n = 32;
  constexpr auto max_n = 1024;
  constexpr auto score = 20;
#else
  constexpr auto min_n = 1024;
  constexpr auto max_n = 4096;
  constexpr auto score = 180;
#endif

  const auto filename = std::string{ "series_" } + std::to_string(min_n) + std::string("_") + std::to_string(max_n) + std::string("_") + std::to_string(score) + std::string(".raw");
  if (!std::filesystem::directory_entry{ filename }.exists()) {
    std::cout << "Creating new problem series..." << std::endl;
    const auto series = generate_problem_series(min_n, max_n, score);

    std::cout << "Writing new problem series..." << std::endl;
    series.serialize(filename);
    for (const auto& instance : series.instances)
      total_time += instance.score;

    std::cout << "New problem series: min n " << min_n << ", max n " << max_n << ", total score " << std::fixed << total_time << "s" << std::endl;
  } else {
    std::cout << "Reading problem series..." << std::endl;
    const auto [instances] = LinearEquationSeries::deserialize(filename);
    for (const auto& instance : instances)
      total_time += instance.score;
    std::cout << "Problem series: min n " << min_n << ", max n " << max_n << ", total score " << std::fixed << total_time << "s" << std::endl;
  }

  std::vector<std::pair<std::string, std::function<std::shared_ptr<LinearEquationSchedulerBase>(std::size_t)>>> schedulers{
    { "mixed", [=](std::size_t num_threads) { return std::make_shared<LES_mixed>(problem_queue, queue_mutex, queue_cv, num_threads); } },
    { "parallel", [=](std::size_t num_threads) { return std::make_shared<LES_parallel>(problem_queue, queue_mutex, queue_cv, num_threads); } },
    { "trivial", [=](std::size_t num_threads) { return std::make_shared<LES_trivial>(problem_queue, queue_mutex, queue_cv, num_threads); } },
  };
  constexpr auto map_load = [](double load_factor) {
    auto num_threads = static_cast<unsigned>(2.0 * load_factor);
    num_threads = std::max(1u, num_threads);
    num_threads = std::min(num_threads, std::thread::hardware_concurrency() - 1u);
    return std::pair{ num_threads, load_factor };
  };

  std::vector load_factors{
    map_load(8.0),
    map_load(4.0),
    map_load(2.0),
    map_load(1.0),
    map_load(0.9),

    // optional extremes
    map_load(10.0),
    map_load(0.95),

    // optional more data points
    map_load(6.0),
    map_load(3.0),
    map_load(1.5),
  };
  const auto les = LinearEquationSeries::deserialize(filename);

  for (const auto [num_threads, load_factor] : load_factors) {
    for (const auto& [name, make_scheduler] : schedulers) {

      std::cout << std::endl;
      std::cout << "[Main] Start ";
      std::cout << "[scheduler:" << name << "]";
      std::cout << "[threads:" << num_threads << "]";
      std::cout << "[score:" << std::fixed << total_time << "s]";
      std::cout << "[load:" << std::fixed << (100.0 * load_factor) << "%]" << std::endl;

      double elapsed;
      {
        DEBUG_ASSERT(problem_queue->empty());

        // Create and start the problem producer and scheduler.
        auto producer = LinearEquationSeriesProducer{ les, load_factor, problem_queue, queue_mutex, queue_cv };
        const auto scheduler = make_scheduler(num_threads);
        producer.start();

        const auto start = std::chrono::steady_clock::now();
        scheduler->start();
        producer.join();   // wait for the producer to finish
        scheduler->stop(); // signal to stop when queue is empty
        const auto end = std::chrono::steady_clock::now();

        elapsed = std::chrono::duration<double>(end - start).count();
      } // ensure cleanup before report

      std::cout.flush();
      std::cout << "[Main] Finished ";
      std::cout << "[scheduler:" << name << "]";
      std::cout << "[time:" << std::fixed << elapsed << "s]";
      std::cout << "[efficiency:" << std::fixed << 100.0 * ((total_time / num_threads) / elapsed) << "%]";
      std::cout << "[threads:" << num_threads << "]";
      std::cout << "[score:" << std::fixed << total_time << "s]";
      std::cout << "[load:" << std::fixed << (100.0 * load_factor) << "%]" << std::endl;
    }
  }
}
