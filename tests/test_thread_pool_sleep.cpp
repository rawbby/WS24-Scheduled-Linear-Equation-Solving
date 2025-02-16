#include <assert.hpp>

#include <cstddef>
#include <thread_pool.hpp>

class SleepTask final : public Task
{
  std::uint64_t sleep_time_ns;

public:
  explicit SleepTask(std::uint64_t sleep_time_ns)
    : sleep_time_ns(sleep_time_ns)
  {
  }

  ~SleepTask() override = default;

  void
  operator()(std::size_t tid) override
  {
    std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_time_ns));
  }
};

int
main()
{
  thread_dump_suffix_ = "test_thread_pool_";
  const auto num_tasks = static_cast<std::size_t>(std::thread::hardware_concurrency()) * 64;
  constexpr std::uint64_t sleep_time_ns = 1'000'000'000;

  auto tasks = std::vector<std::shared_ptr<Task>>{};
  for (int i = 0; i < num_tasks; ++i)
    tasks.emplace_back(std::make_shared<SleepTask>(sleep_time_ns));

  START(SleepTask);
  auto pool = ThreadPool{ num_tasks };
  for (const auto& task : tasks)
    pool.enqueue(task);
  pool.stop();
  const auto elapsed = END(SleepTask);

  // assume less than 100ms overhead
  std::cout << elapsed << "ns" << std::endl;
  ASSERT_LE(elapsed, sleep_time_ns + 200'000'000);
  ASSERT_GE(elapsed, sleep_time_ns);
}
