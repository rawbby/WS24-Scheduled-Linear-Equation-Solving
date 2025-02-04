#include "./test_util.hpp"

int
main()
{
  ASSERT(true);
  ASSERT_EQ(0, 0);
  ASSERT_NE(0, 1);
  ASSERT_LE(0, 0);
  ASSERT_LE(0, 1);
  ASSERT_LT(0, 1);
  ASSERT_GE(0, 0);
  ASSERT_GE(1, 0);
  ASSERT_GT(1, 0);

  ASSERT(true, "some test message %s", "test");
  ASSERT_EQ(0, 0, "some test message %s", "test");
  ASSERT_NE(0, 1, "some test message %s", "test");
  ASSERT_LE(0, 0, "some test message %s", "test");
  ASSERT_LE(0, 1, "some test message %s", "test");
  ASSERT_LT(0, 1, "some test message %s", "test");
  ASSERT_GE(0, 0, "some test message %s", "test");
  ASSERT_GE(1, 0, "some test message %s", "test");
  ASSERT_GT(1, 0, "some test message %s", "test");
}
