#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

extern "C" void fzn_blackbox(const int *int_in, size_t int_in_size,
                             const double *float_in, size_t float_in_size,
                             int *int_out, size_t int_out_size,
                             double *float_out, size_t float_out_size) {

  assert(int_out_size >= 1);
  assert(int_in_size >= 2);

  const int64_t scale = static_cast<int64_t>(int_in[int_in_size - 1]);
  const size_t n = int_in_size - 1;

  int64_t m = 0;
  for (size_t i = 0; i < n; ++i) {
    m += static_cast<int64_t>(int_in[i]);
  }
  assert(m != 0);

  int64_t diff_sum = 0;
  for (size_t i = 0; i < n; ++i) {
    const int64_t xi = static_cast<int64_t>(int_in[i]);
    for (size_t j = i + 1; j < n; ++j) {
      const int64_t xj = static_cast<int64_t>(int_in[j]);
      const int64_t d = xi - xj;
      diff_sum += (d >= 0) ? d : -d;
    }
  }

  const int64_t tot_diff = diff_sum * scale;
  const int64_t result_ = tot_diff / static_cast<int64_t>(n);
  const int64_t result = std::llabs(result_) / std::llabs(m);
  int_out[0] = static_cast<int>(result);
}
