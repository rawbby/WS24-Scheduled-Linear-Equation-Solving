#include "./test_util.hpp"

#include <thread_pool.hpp>

class Sleep final : public Task
{
  int millis_;

public:
  explicit Sleep(int millis)
    : millis_(millis)
  {
  }

  ~Sleep() override = default;

  void
  operator()() override
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(millis_));
  }
};

int
main()
{
  constexpr int num_tasks = 512;
  constexpr int sleep_time = 1000;

  auto pool = ThreadPool{ num_tasks };
  auto tasks = std::vector<std::shared_ptr<Task>>{};

  const auto start = std::chrono::steady_clock::now();

  for (int i = 0; i < num_tasks; ++i)
    pool.enqueue(tasks.emplace_back(std::make_shared<Sleep>(sleep_time)));

  for (const auto& task : tasks)
    pool.await(task);

  const auto end = std::chrono::steady_clock::now();
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  // assume less than 100ms overhead
  ASSERT_LE(elapsed, sleep_time + 100);
  ASSERT_GE(elapsed, sleep_time);
}
