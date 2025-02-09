#include <assert.hpp>

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
  operator()(std::size_t tid) override
  {
    result_ = input_ + input_;
  }

  int
  result() const
  {
    return result_;
  }
};

int
main()
{
  auto pool = ThreadPool{ 4 };
  const auto task = std::make_shared<MultiplyByTwo>(21);
  pool.enqueue(task);
  pool.stop();
  ASSERT_EQ(task->result(), 21 * 2);
}
