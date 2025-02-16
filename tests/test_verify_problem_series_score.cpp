#include <LES_verification.hpp>
#include <linear_equation_series.hpp>
#include <linear_equation_series_producer.hpp>

#include <assert.hpp>

#include <queue>

void verify_a()
{
  const auto problem_queue = std::make_shared<std::queue<LinearEquation>>();
  const auto queue_mutex = std::make_shared<std::mutex>();
  const auto queue_cv = std::make_shared<std::condition_variable>();

  const auto series = generate_problem_series(1024, 2048, 6.0);

  auto producer = LinearEquationSeriesProducer{ std::move(series), std::numeric_limits<double>::max(), problem_queue, queue_mutex, queue_cv };
  auto verifier = LES_verification_a{ problem_queue, queue_mutex, queue_cv };

  producer.start();
  verifier.start();
  producer.join(); // wait for the producer to finish
  verifier.stop(); // signal to stop when queue is empty

  ASSERT_GT(verifier.success_percent(), 95.0);
}

void verify_b()
{
  const auto problem_queue = std::make_shared<std::queue<LinearEquation>>();
  const auto queue_mutex = std::make_shared<std::mutex>();
  const auto queue_cv = std::make_shared<std::condition_variable>();

  const auto series = generate_problem_series(1024, 2048, 6.0);

  auto producer = LinearEquationSeriesProducer{ std::move(series), std::numeric_limits<double>::max(), problem_queue, queue_mutex, queue_cv };
  auto verifier = LES_verification_b{ problem_queue, queue_mutex, queue_cv };

  producer.start();
  verifier.start();
  producer.join(); // wait for the producer to finish
  verifier.stop(); // signal to stop when queue is empty

  ASSERT_GT(verifier.success_percent(), 80.0);
}

int
main()
{
  verify_a();
  verify_b();
}
