#include <minizinc/minizinc_eval.hh>

MiniZinc::MznEvaluator e;
bool initialized = false;
const std::string model = R"(

% ν-centred assignment θν (Definition 2)
function float: theta(tuple(int, int): x, float: v) =
  if x.2 <= v then
    x.2
  elseif x.1 >= v then
    x.1
  else
    v
  endif;

% LBV'(X, v) = (1/n) * sum_i (theta(x_i, v) - v)^2
function float: lbv_prime(array[int] of tuple(int, int): X, float: v) =
  sum(i in index_set(X))(
    pow(theta(X[i], v) - v, 2)
  ) / length(X);


function int: spread_lb(array[int] of tuple(int, int): X, tuple(int, int): M, int: s) =
  let {
    int: m_l = M.1 + ((M.2 - M.1) div 2);
    int: m_r = m_l + 1;
    float: V_l = lbv_prime(X, m_l / length(X));
    float: V_r = lbv_prime(X, m_r / length(X));
  } in
  if V_l = 0 \/ V_r = 0 then
    0
  elseif V_l = V_r then
    floor(V_l * s)
  elseif V_l < V_r then
    if M.1 >= m_l then
      floor(V_l * s)
    else
      spread_lb(X, (M.1, m_l), s)
    endif
  elseif m_r >= M.2 then
    floor(V_r * s)
  else
    spread_lb(X, (m_r, M.2), s)
  endif;

function tuple(array[int] of int, array[int] of float): main(
		array[int] of int: x,
		array[int] of float: y
) = ([spread_lb([(x[i*2+1], x[i*2+2]) | i in 0..<(length(x) div 2 -1)], (x[length(x)-1], x[length(x)]), 1),], []);

)";

extern "C" void fzn_blackbox(const int * int_in, size_t int_in_size, const double * float_in, size_t float_in_size, int * int_out, size_t int_out_size, double * float_out, size_t float_out_size) {
	if (!initialized) {
		e.loadModelString(model);
		initialized = true;
	}

	std::vector<long long> param(int_in_size - 2);
	for (size_t i = 0; i < int_in_size - 2; ++i) {
		param[i] = int_in[i];
	}
	auto out = e.eval({param, {}});

	assert(int_in_size == int_out_size);
	for (size_t i = 0; i < int_out_size; ++i) {
		int_out[i] = int_in[i];
	}
	int_out[int_out_size - 2] = static_cast<int>(out.first[0]);
}
