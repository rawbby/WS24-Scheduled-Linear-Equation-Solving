# sudo cpupower frequency-set -g userspace
# sudo cpupower frequency-set -d 2.0GHz -u 2.0GHz

#scheduler_name="trivial"
num_threads=6
load_factor="600.0"
min_n=32
max_n=1024
score="200.0"

cd ../cmake-build-release/bin
sudo chrt -f 99 perf stat -e cache-misses,branch-misses,cycles,instructions ./bench_online_scheduling "trivial" $num_threads $load_factor $min_n $max_n $score
sudo chrt -f 99 perf stat -e cache-misses,branch-misses,cycles,instructions ./bench_online_scheduling "parallel" $num_threads $load_factor $min_n $max_n $score
sudo chrt -f 99 perf stat -e cache-misses,branch-misses,cycles,instructions ./bench_online_scheduling "mixed" $num_threads $load_factor $min_n $max_n $score
