# Online Linear Equation Scheduling

---

## Building the Project

### Prerequisites

- A C++ compiler (supporting at least C++20)
- [CMake](https://cmake.org/) for building
- [Python3](https://python.org/) for benchmarks and plotting

### Dependencies

- The project uses my own cmake-utilities which are automatically downloaded by the CMakeLists.txt

### Build Steps

1. Generate the build system (Release configuration):
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release -S . -B cmake-build-release
   ```
2. Build the project:
   ```bash
   cmake --build cmake-build-release --target all
   ```

The executables will be located in `cmake-build-release/bin/`.

---

## Running the Program

Have a look at bench_online_scheduling/bench script

### Output

- The program prints performance results (construction time, memory usage, query time) to **stdout**.
- It also writes the line-by-line results of the queries to a file named `result_<eingabe_datei>` in the current
  directory.
