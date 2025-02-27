# sudo cpupower frequency-set -g userspace
# sudo cpupower frequency-set -d 2.0GHz -u 2.0GHz

#scheduler_name="trivial"
num_threads=4
load_factor="2.4"
min_n=768
max_n=4096
score="120.0"

#perf stat -e cache-misses,branch-misses,cycles,instructions

cp list_suffixes.py ../cmake-build-release/bin/list_suffixes.py
cp gantt_plot.py ../cmake-build-release/bin/gantt_plot.py

cd ../cmake-build-release/bin
# sudo chrt -f 99 ./bench_online_scheduling "trivial" $num_threads $load_factor $min_n $max_n $score
# sudo chrt -f 99 ./bench_online_scheduling "parallel" $num_threads $load_factor $min_n $max_n $score
# sudo chrt -f 99 ./bench_online_scheduling "mixed" $num_threads $load_factor $min_n $max_n $score

../../.venv/bin/python3 list_suffixes.py
# ../../.venv/bin/python3 gantt_plot.py mixed_4_2.400000_768_4096_120
# ../../.venv/bin/python3 gantt_plot.py parallel_4_2.400000_768_4096_120
../../.venv/bin/python3 gantt_plot.py trivial_4_2.400000_768_4096_120
