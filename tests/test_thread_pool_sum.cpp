#include <assert.hpp>

#include <thread_pool.hpp>

class Id final : public Task
{
  int input_;
  int result_;

public:
  explicit Id(int input)
    : input_(input)
    , result_()
  {
  }

  ~Id() override = default;

  void
  operator()(std::size_t tid) override
  {
    result_ = input_;
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
  constexpr int num_tasks = 100;
  constexpr int expected_sum = num_tasks * (num_tasks - 1) / 2;

  auto pool = ThreadPool{ 4 };
  auto tasks = std::vector<std::shared_ptr<Id>>{};
  for (int i = 0; i < num_tasks; ++i)
    pool.enqueue(tasks.emplace_back(std::make_shared<Id>(i)));

  pool.stop();

  int sum = 0;
  for (const auto& task : tasks)
    sum += task->result();

  ASSERT_EQ(sum, expected_sum);
}
