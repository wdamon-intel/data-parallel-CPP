// Copyright (C) 2020 Intel Corporation

// SPDX-License-Identifier: MIT

#include <CL/sycl.hpp>
#include <algorithm>
#include <iostream>
#include <random>
using namespace sycl;

int main() {
  // Set up queue on any available device
  queue Q;

  // Initialize input and output memory on the host
  constexpr size_t N = 256;
  constexpr size_t B = 8;
  std::vector<float> a(N * N), b(N * N), c(N * N);
  std::default_random_engine gen(42);
  std::uniform_real_distribution<float> dist(0.0, 1.0);
  auto rng = [&]() {
    return dist(gen);
  };
  std::generate(a.begin(), a.end(), rng);
  std::generate(b.begin(), b.end(), rng);
  std::fill(c.begin(), c.end(), 0);

  {
    // Create buffers associated with inputs and output
    buffer<float, 2> a_buf(a.data(), range<2>(N, N)),
        b_buf(b.data(), range<2>(N, N)), c_buf(c.data(), range<2>(N, N));

    // Submit the kernel to the queue
    Q.submit([&](handler& h) {
      accessor a{a_buf, h};
      accessor b{b_buf, h};
      accessor c{c_buf, h};

// START CODE SNIP
      range num_groups{N / B, N / B}; // N is a multiple of B
      range group_size{B, B};
      h.parallel_for_work_group(num_groups, [=](group<2> grp) {
        int jb = grp.get_id(0);
        int ib = grp.get_id(1);
        grp.parallel_for_work_item(group_size, [&](h_item<2> it) {
          int j = jb * B + it.get_logical_local_id(0);
          int i = ib * B + it.get_logical_local_id(1);
          for (int k = 0; k < N; ++k) {
            c[j][i] += a[j][k] * b[k][i];
          }
        });
      });
// END CODE SNIP
    });
  }

  // Check that all outputs match serial execution.
  bool passed = true;
  for (int j = 0; j < N; ++j) {
    for (int i = 0; i < N; ++i) {
      float gold = 0;
      for (int k = 0; k < N; ++k) {
        gold += a[j * N + k] * b[k * N + i];
      }
      if (std::abs(gold - c[j * N + i]) / gold > 1.0E-05) {
        passed = false;
      }
    }
  }
  std::cout << ((passed) ? "SUCCESS" : "FAILURE") << std::endl;
  return (passed) ? 0 : 1;
}
