#include "./test_util.hpp"

#include <thread_pool.hpp>

class MultiplyByTwo final : public Task
{
  const int input_;
  int result_;

public:
  explicit MultiplyByTwo(int input)
    : input_(input)
    , result_()
  {
  }

  ~MultiplyByTwo() override = default;

  void
  operator()() override
  {
    result_ = 2 * input_;
  }

  int
  result() const
  {
    return result_;
  }
};

int
multiply_by_two(int value)
{
  return value * 2;
}

int
main()
{
  auto pool = ThreadPool{ 4 };
  auto task = std::make_shared<MultiplyByTwo>(21);
  pool.enqueue(task);
  pool.await(task);
  ASSERT_EQ(task->result(), 21 * 2);
}
