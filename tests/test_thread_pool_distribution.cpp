#include "./test_util.hpp"

#include <thread_pool.hpp>

class ThreadId final : public Task
{
  int result_ = 0;

public:
  ~ThreadId() override = default;

  void
  operator()() override
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    result_ = static_cast<int>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
  }

  int
  result()
  {
    return result_;
  }
};

int
main()
{
  constexpr int num_tasks = 256;
  constexpr int num_threads = 128;

  auto pool = ThreadPool{ num_threads };
  auto tasks = std::vector<std::shared_ptr<ThreadId>>{};
  for (int i = 0; i < num_tasks; ++i)
    pool.enqueue(tasks.emplace_back(std::make_shared<ThreadId>()));

  for (const auto& task : tasks)
    pool.await(task);

  std::vector<int> unique_ids;
  for (const auto& task : tasks)
    if (std::ranges::find(unique_ids, task->result()) == unique_ids.end())
      unique_ids.push_back(task->result());

  // await also runs tasks; therefore + 1
  ASSERT_EQ(unique_ids.size(), num_threads + 1);
}
