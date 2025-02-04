#include <linear_equation.hpp>
#include <linear_equation_scheduler.hpp>
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

  if (!std::filesystem::directory_entry{ "series.raw" }.exists()) {
    const auto series = generate_problem_series(8, 512, 30.0);
    series.serialize("series.raw");
    for (const auto& instance : series.instances)
      total_time += instance.score;
  } else {
    const auto [instances] = LinearEquationSeries::deserialize("series.raw");
    for (const auto& instance : instances)
      total_time += instance.score;
  }

  // Create and start the problem producer and scheduler.
  LinearEquationSeriesProducer producer("series.raw", problem_queue, queue_mutex, queue_cv);
  LinearEquationScheduler scheduler(problem_queue, queue_mutex, queue_cv);

  producer.start();
  const auto start = std::chrono::steady_clock::now();
  scheduler.start();

  producer.join();  // wait for the producer to finish
  scheduler.stop(); // signal to stop when queue is empty

  const auto end = std::chrono::steady_clock::now();
  const double elapsed = std::chrono::duration<double>(end - start).count();

  std::cout << "Took " << elapsed << " seconds. Total score " << total_time << " seconds." << std::endl;
  std::cout << "Scheduler stopped. Exiting.\n";
}
