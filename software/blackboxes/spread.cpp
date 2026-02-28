#include <cassert>
#include <cstddef>
#include <cstdint>

extern "C" void fzn_blackbox(const int *int_in, size_t int_in_size,
                             const double *float_in, size_t float_in_size,
                             int *int_out, size_t int_out_size,
                             double *float_out, size_t float_out_size) {
  assert(int_out_size == 1);

  constexpr int64_t scale = 1;
  const int64_t n = static_cast<int64_t>(int_in_size);

  int64_t sum = 0;
  for (size_t i = 0; i < int_in_size; ++i) {
    sum += static_cast<int64_t>(int_in[i]);
  }

  int64_t s = 0;
  for (size_t i = 0; i < int_in_size; ++i) {
    const int64_t term = n * static_cast<int64_t>(int_in[i]) - sum;
    s += term * term;
  }

  const int64_t numerator = s * scale;
  const int64_t denominator = n * n * n;

  int_out[0] = static_cast<int>(numerator / denominator);
}
