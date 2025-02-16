set num_threads_=3
set load_factor=2.0
set min_n=1024
set max_n=3072
set score=60.0

cd ..
cd cmake-build-release
cd bin
cd Release

dir
start /wait /realtime bench_online_scheduling.exe "trivial" 1 1000000000 %min_n% %max_n% %score%
start /wait /realtime bench_online_scheduling.exe "parallel" 3 1000000000 %min_n% %max_n% %score%
start /wait /realtime bench_online_scheduling.exe "mixed" 3 1000000000 %min_n% %max_n% %score%

rem C:\Users\user\workspace\online_scheduling\.venv\Scripts\python.exe -m ensurepip
rem C:\Users\user\workspace\online_scheduling\.venv\Scripts\python.exe -m pip install --upgrade numpy matplotlib

C:\Users\user\workspace\online_scheduling\.venv\Scripts\python.exe ..\..\..\bench_online_scheduling\gantt_plot.py  trivial_1_1000000000.000000_1024_3072_60
C:\Users\user\workspace\online_scheduling\.venv\Scripts\python.exe ..\..\..\bench_online_scheduling\gantt_plot.py  parallel_3_1000000000.000000_1024_3072_60
C:\Users\user\workspace\online_scheduling\.venv\Scripts\python.exe ..\..\..\bench_online_scheduling\gantt_plot.py  mixed_3_1000000000.000000_1024_3072_60
