#pragma once

#include "./linear_equation.hpp"
#include "solver.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ios>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class LinearEquationSeries
{
public:
  std::vector<LinearEquation> instances;

  void
  serialize(const std::string& filename) const
  {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs)
      throw std::runtime_error("failed to open file " + filename);

    const std::size_t count = instances.size();
    {
      std::array<char, sizeof(count)> buffer{};
      std::memcpy(buffer.data(), &count, buffer.size());
      ofs.write(buffer.data(), buffer.size());
    }
    for (const auto& inst : instances) {
      {
        std::array<char, sizeof(inst.n)> buffer{};
        std::memcpy(buffer.data(), &inst.n, buffer.size());
        ofs.write(buffer.data(), buffer.size());
      }
      {
        std::array<char, sizeof(inst.score)> buffer{};
        std::memcpy(buffer.data(), &inst.score, buffer.size());
        ofs.write(buffer.data(), buffer.size());
      }
      {
        const std::size_t a_size = inst.n * inst.n;
        std::vector<char> buffer(a_size * sizeof(double));
        std::memcpy(buffer.data(), inst.A.data(), buffer.size());
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      }
      {
        const std::size_t b_size = inst.n;
        std::vector<char> buffer(b_size * sizeof(double));
        std::memcpy(buffer.data(), inst.b.data(), buffer.size());
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      }
    }
    ofs.close();
  }

  static LinearEquationSeries
  deserialize(const std::string& filename)
  {
    LinearEquationSeries series;
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
      throw std::runtime_error("failed to open file");

    std::size_t count = 0;
    {
      std::array<char, sizeof(count)> buffer{};
      ifs.read(buffer.data(), buffer.size());
      std::memcpy(&count, buffer.data(), buffer.size());
    }
    series.instances.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
      LinearEquation inst;
      {
        std::array<char, sizeof(inst.n)> buffer{};
        ifs.read(buffer.data(), buffer.size());
        std::memcpy(&inst.n, buffer.data(), buffer.size());
      }
      {
        std::array<char, sizeof(inst.score)> buffer{};
        ifs.read(buffer.data(), buffer.size());
        std::memcpy(&inst.score, buffer.data(), buffer.size());
      }
      inst.A.resize(inst.n * inst.n);
      inst.b.resize(inst.n);
      {
        const std::size_t a_size = inst.n * inst.n;
        std::vector<char> buffer(a_size * sizeof(double));
        ifs.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        std::memcpy(inst.A.data(), buffer.data(), buffer.size());
      }
      {
        const std::size_t b_size = inst.n;
        std::vector<char> buffer(b_size * sizeof(double));
        ifs.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        std::memcpy(inst.b.data(), buffer.data(), buffer.size());
      }
      series.instances[i] = std::move(inst);
    }
    ifs.close();
    return series;
  }
};

[[nodiscard]] inline LinearEquationSeries
generate_problem_series(std::size_t min_n, std::size_t max_n, double min_total_score)
{
  auto engine = std::mt19937_64{ std::random_device{}() };
  auto distribution = std::uniform_int_distribution{ min_n, max_n };

  LinearEquationSeries series;
  double total_score = 0.0;

  while (total_score < min_total_score) {
    const auto n = distribution(engine);
    auto inst = generate_diagonally_dominant_system(n);

    const auto beg = std::chrono::steady_clock::now();
    [[maybe_unused]] const auto sol = lu_solver::solve(inst);
    const auto end = std::chrono::steady_clock::now();

    const auto score = std::chrono::duration<double>(end - beg).count();
    inst.score = score;
    series.instances.push_back(std::move(inst));
    total_score += score;
  }
  return series;
}
