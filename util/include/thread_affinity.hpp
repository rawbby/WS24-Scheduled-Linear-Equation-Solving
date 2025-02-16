#pragma once

#include "./assert.hpp"

#include <thread>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#include <sched.h>
#else
#error "Unsupported platform"
#endif

static void
pin_to_core(int core_id)
{
  ASSERT(core_id >= 0, "Core ID must be non-negative");

#ifdef _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  DWORD_PTR mask = (1ULL << (core_id % sysinfo.dwNumberOfProcessors));
  HANDLE thread = GetCurrentThread();
  ASSERT(SetThreadAffinityMask(thread, mask) != 0, "Failed to set thread affinity");
#elif defined(__linux__) || defined(__APPLE__)
  core_id %= sysconf(_SC_NPROCESSORS_ONLN);
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);
  ASSERT_EQ(0, pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset), "failed to set thread affinity");
#endif
}
