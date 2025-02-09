#pragma once

#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct LinearEquation
{
  std::size_t n{};
  std::vector<double> A; // Coefficient matrix in row-major order (size n*n)
  std::vector<double> b; // Right-hand side vector (size n)
  double score = 0.0;    // Time (in seconds) taken by the solver

  LinearEquation() = default;
  ~LinearEquation() = default;

  explicit LinearEquation(std::size_t n)
    : n(n)
    , A(n * n)
    , b(n)
  {
  }

  LinearEquation(std::size_t n, std::vector<double>&& A, std::vector<double>&& b, double score = 0.0)
    : n(n)
    , A(A)
    , b(b)
    , score(score)
  {
  }

  LinearEquation(const LinearEquation& other)

    = default;

  LinearEquation(LinearEquation&& other) noexcept
    : n(other.n)
    , A(std::move(other.A))
    , b(std::move(other.b))
    , score(other.score)
  {
    other.n = 0;
    other.score = 0.0;
  }

  LinearEquation&
  operator=(const LinearEquation& other)
  {
    if (this != &other) {
      n = other.n;
      A = other.A;
      b = other.b;
      score = other.score;
    }
    return *this;
  }

  LinearEquation&
  operator=(LinearEquation&& other) noexcept
  {
    if (this != &other) {
      n = other.n;
      A = std::move(other.A);
      b = std::move(other.b);
      score = other.score;

      other.n = 0;
      other.score = 0.0;
    }
    return *this;
  }

  void
  serialize(const std::string& filename) const
  {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs)
      throw std::runtime_error("failed to open file");
    {
      std::array<char, sizeof(n)> buffer{};
      std::memcpy(buffer.data(), &n, buffer.size());
      ofs.write(buffer.data(), buffer.size());
    }
    {
      std::array<char, sizeof(score)> buffer{};
      std::memcpy(buffer.data(), &score, buffer.size());
      ofs.write(buffer.data(), buffer.size());
    }
    {
      std::vector<char> buffer(A.size() * sizeof(double));
      std::memcpy(buffer.data(), A.data(), buffer.size());
      ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
    {
      std::vector<char> buffer(b.size() * sizeof(double));
      std::memcpy(buffer.data(), b.data(), buffer.size());
      ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
  }

  static LinearEquation
  deserialize(const std::string& filename)
  {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
      throw std::runtime_error("failed to open file");

    LinearEquation instance;
    {
      std::array<char, sizeof(instance.n)> buffer{};
      ifs.read(buffer.data(), buffer.size());
      std::memcpy(&instance.n, buffer.data(), buffer.size());
    }
    {
      std::array<char, sizeof(instance.score)> buffer{};
      ifs.read(buffer.data(), buffer.size());
      std::memcpy(&instance.score, buffer.data(), buffer.size());
    }
    instance.A.resize(instance.n * instance.n);
    instance.b.resize(instance.n);
    {
      std::vector<char> buffer(instance.A.size() * sizeof(double));
      ifs.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      std::memcpy(instance.A.data(), buffer.data(), buffer.size());
    }
    {
      std::vector<char> buffer(instance.b.size() * sizeof(double));
      ifs.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      std::memcpy(instance.b.data(), buffer.data(), buffer.size());
    }
    return instance;
  }
};

inline LinearEquation
generate_diagonally_dominant_system(std::size_t n)
{
  auto engine = std::mt19937_64{ std::random_device{}() };
  auto distribution = std::uniform_real_distribution{ -1.0, 1.0 };

  auto le = LinearEquation{ n };
  for (std::size_t i = 0; i < n; ++i) {
    double row_sum = 0.0;
    for (std::size_t j = 0; j < n; ++j) {
      if (i == j)
        continue;
      const double val = distribution(engine);
      le.A[(i * n) + j] = val;
      row_sum += std::abs(val);
    }
    // Set diagonal to ensure strict diagonal dominance
    le.A[(i * n) + i] = row_sum + 1.0;
    le.b[i] = distribution(engine);
  }
  return le;
}
