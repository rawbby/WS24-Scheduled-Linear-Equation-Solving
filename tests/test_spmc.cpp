#include <fixed_mpsc.hpp>
#include <fixed_spmc.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <ranges>
#include <thread>

template<typename Q>
void
test_push_pop()
{
  Q q{ 0x10 };
  const auto i = std::make_shared<int>(1);
  ASSERT(q.try_push(i));

  const auto j_ = q.try_pop();
  ASSERT(j_.has_value());
  const auto j = j_.value();

  ASSERT_EQ(i, j);
  ASSERT_EQ(*i, *j);
  ASSERT_NE(i, nullptr);
  ASSERT_NE(j, nullptr);
}

int
main()
{
  using namespace std::chrono_literals;
  test_push_pop<FixedSPMC<int>>();
  test_push_pop<FixedMPSC<int>>();

  constexpr auto capacity_spmc = 0x10;
  constexpr auto capacity_mpsc = 0x100;

  FixedSPMC<int> spmc{ capacity_spmc };
  FixedMPSC<int> mpsc{ capacity_mpsc };

  {
    std::atomic_bool all_produced_signal = false;

    std::jthread producer([&] {
      for (int i = 0; i < capacity_mpsc; ++i) {
        while (!spmc.try_push(std::make_shared<int>(i)))
          std::this_thread::sleep_for(100000ns);
        std::this_thread::sleep_for(100000ns);
      }
      all_produced_signal = true;
      std::cout << "All Items produced!" << std::endl;
    });

    {
      std::vector<std::jthread> consumer{};
      consumer.reserve(0x10);
      for (int i = 0; i < 0x10; ++i) {
        consumer.emplace_back([&, i] {
          while (!all_produced_signal || spmc.size()) {
            const auto item = spmc.try_pop();
            if (item.has_value()) {
              while (!mpsc.try_push(item.value()))
                std::this_thread::sleep_for(200000ns);
            }
          }
          std::cout << "Consumer " << i << " detected stop on empty Queue!" << std::endl;
        });
      }
    }
  }

  std::cout << "Production and Consumption Phase finished!" << std::endl;
  std::cout << "evaluating ..." << std::endl;

  ASSERT(spmc.empty());
  ASSERT_EQ(mpsc.size(), capacity_mpsc);

  std::vector<int> data(capacity_mpsc);
  for (int i = 0; i < capacity_mpsc; ++i) {
    const auto item = mpsc.try_pop();
    ASSERT(item.has_value());
    data[i] = *item.value();
  }

  ASSERT(mpsc.empty());

  std::ranges::sort(data, std::less<int>{});
  for (int i = 0; i < capacity_mpsc; ++i)
    ASSERT_EQ(data[i], i);
}
