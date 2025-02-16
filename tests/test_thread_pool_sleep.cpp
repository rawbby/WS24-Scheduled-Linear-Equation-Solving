#include <assert.hpp>

#include <thread_pool.hpp>

class Sleep final : public Task
{
  std::uint64_t sleep_time_ns;

public:
  explicit Sleep(std::uint64_t sleep_time_ns)
    : sleep_time_ns(sleep_time_ns)
  {
  }

  ~Sleep() override = default;

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
  const std::size_t num_tasks = std::thread::hardware_concurrency() * 64;
  constexpr std::uint64_t sleep_time_ns = 1'000'000'000;

  auto tasks = std::vector<std::shared_ptr<Task>>{};
  for (int i = 0; i < num_tasks; ++i)
    tasks.emplace_back(std::make_shared<Sleep>(sleep_time_ns));

  START(sleep);
  auto pool = ThreadPool{ num_tasks };
  for (const auto& task : tasks)
    pool.enqueue(task);
  pool.stop();
  const auto elapsed = END(sleep);

  // assume less than 100ms overhead
  std::cout << elapsed << "ns" << std::endl;
  ASSERT_LE(elapsed, sleep_time_ns + 100'000'000);
  ASSERT_GE(elapsed, sleep_time_ns);
}
