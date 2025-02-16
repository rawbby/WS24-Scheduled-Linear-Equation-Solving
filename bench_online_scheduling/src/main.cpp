#include <LES_verification.hpp>
#include <les_mixed.hpp>
#include <les_parallel.hpp>
#include <les_size_mixed.hpp>
#include <les_trivial.hpp>
#include <linear_equation.hpp>
#include <linear_equation_series.hpp>
#include <linear_equation_series_producer.hpp>

#include <bench.hpp>

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>

#include <pthread.h>
#include <sched.h>

int
main(int argc, char* argv[])
{
  if (argc != 7) {
    std::cerr << "Usage: " << argv[0] << " <scheduler_name> <num_threads> <load_factor> <min_n> <max_n> <score>\n";
    std::exit(1);
  }

  const std::string scheduler_name = argv[1];
  const unsigned num_threads = std::stoul(argv[2]);
  const double load_factor = std::stod(argv[3]);
  const unsigned min_n = std::stoul(argv[4]);
  const unsigned max_n = std::stoul(argv[5]);
  const unsigned score = std::stoul(argv[6]);

  const auto problem_queue = std::make_shared<std::queue<LinearEquation>>();
  const auto queue_mutex = std::make_shared<std::mutex>();
  const auto queue_cv = std::make_shared<std::condition_variable>();

  const std::string filename = "./series_" + std::to_string(min_n) + "_" + std::to_string(max_n) + "_" + std::to_string(score) + ".raw";
  if (!std::filesystem::exists(filename)) {
    const auto series = generate_problem_series(min_n, max_n, score);
    series.serialize(filename);
  }

  const auto les = LinearEquationSeries::deserialize(filename);
  double total_time = 0.0;
  for (const auto& instance : les.instances)
    total_time += instance.score;

  std::unordered_map<std::string, std::function<std::shared_ptr<LinearEquationSchedulerBase>()>> schedulers{
    { "verification_a", [=] { return std::make_shared<LES_verification_a>(problem_queue, queue_mutex, queue_cv); } },
    { "verification_b", [=] { return std::make_shared<LES_verification_b>(problem_queue, queue_mutex, queue_cv); } },
    { "size_mixed", [=] { return std::make_shared<LES_size_mixed>(problem_queue, queue_mutex, queue_cv, num_threads); } },
    { "mixed", [=] { return std::make_shared<LES_mixed>(problem_queue, queue_mutex, queue_cv, num_threads); } },
    { "trivial", [=] { return std::make_shared<LES_trivial>(problem_queue, queue_mutex, queue_cv, num_threads); } },
    { "parallel", [=] { return std::make_shared<LES_parallel>(problem_queue, queue_mutex, queue_cv, num_threads); } },
  };

  auto make_scheduler = schedulers[scheduler_name];
  if (!make_scheduler) {
    std::cerr << "Scheduler '" << scheduler_name << "' not found.\n";
    return 1;
  }

  thread_dump_suffix_ = scheduler_name + "_" + std::to_string(num_threads) + "_" +
                        std::to_string(load_factor) + "_" + std::to_string(min_n) + "_" +
                        std::to_string(max_n) + "_" + std::to_string(score);

  std::cout << "[Main] Start [scheduler:" << scheduler_name << "][threads:" << num_threads
            << "][load:" << (100.0 * load_factor) << "%][min_n:" << min_n
            << "][max_n:" << max_n << "][score:" << score << "]\n";

  DEBUG_ASSERT(problem_queue->empty());
  auto producer = LinearEquationSeriesProducer{ les, load_factor, problem_queue, queue_mutex, queue_cv };
  producer.start();

  const auto scheduler = make_scheduler();
  START(bench);
  scheduler->start();
  producer.join();
  scheduler->stop();
  const auto duration = END(bench);

  std::cout << "[Main] Finished [scheduler:" << scheduler_name << "][time:" << std::fixed
            << duration << "s][efficiency:" << std::fixed
            << 100.0 * ((total_time / num_threads) / duration) << "%][threads:" << num_threads
            << "][load:" << (100.0 * load_factor) << "%]\n";
}
