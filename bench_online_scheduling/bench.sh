# sudo cpupower frequency-set -g userspace
# sudo cpupower frequency-set -d 2.0GHz -u 2.0GHz
# sudo chrt -f 99 perf stat -e cache-misses,branch-misses,cycles,instructions ./bench_online_scheduling
