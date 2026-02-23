#include <minizinc/minizinc_eval.hh>

#include <iostream>
#include <string>

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

% Gini(X, ν) for the ν-centred assignment, using Eq. (2) on the sorted θν-values
function float: gini_inner(array[int] of tuple(int, int): X, float: v) =
 let {
   any: t = [theta(x, v) | x in X];
   any: denom = length(X) * sum(t);
 } in
 if denom = 0 then
   0
 else
   sum(i in index_set(X), j in i..max(index_set(X)))(
     abs(t[i] - t[j])
   ) / denom
 endif;

% Move l left while Gini(X, βl) = Gr, stopping either at L or when it differs.
% Returns (l', Gl').
function tuple(int, float): gini_move_left(
   array[int] of tuple(int,int): X,
   array[int] of float: B,
   int: L,
   int: l,
   float: Gr
) =
 let { float: Gl = gini_inner(X, B[l]) } in
 if (Gl = Gr) /\ (l > L) then
   gini_move_left(X, B, L, l - 1, Gr)
 else
   (l, Gl)
 endif;

% Binary chop from Algorithm 2, but returning the index m (= R at termination)
function int: gini_argmin_index(
   array[int] of tuple(int,int): X,
   array[int] of float: B,
   int: L,
   int: R
) =
 if L >= R then
   R
 else
   let {
     int: l0 = L + floor((R - L) / 2);
     int: r0 = l0 + 1;
     float: Gl0 = gini_inner(X, B[l0]);
     float: Gr0 = gini_inner(X, B[r0])
   } in if Gl0 = Gr0 then
     if l0 > L then
       let {
         tuple(int, float): t = gini_move_left(X, B, L, l0 - 1, Gr0);
         int: l = t.1;
         float: Gl = t.2
       } in
         if Gl = Gr0 /\ l = L then
           % matches: else { L <- r; break } then continue with (L=r, R)
           gini_argmin_index(X, B, r0, R)
         elseif Gl < Gr0 then
           gini_argmin_index(X, B, L, l)
         else
           gini_argmin_index(X, B, r0, R)
         endif
     else
       % l == L: set L <- r and continue
       gini_argmin_index(X, B, r0, R)
     endif
   elseif Gl0 < Gr0 then
     gini_argmin_index(X, B, L, l0)
   else
     gini_argmin_index(X, B, r0, R)
   endif
 endif;

function int: gini_lb(array[int] of tuple(int,int): X, int: s) =
 let {
   int: n = length(X);
   array[1..2*n] of float: B =
     sort(
       [ X[i].1 | i in 1..n ] ++
       [ X[i].2 | i in 1..n ]
     );
   int: m = gini_argmin_index(X, B, 1, 2*n);
   float: G = gini_inner(X, B[m])
 } in
 floor(G * s);

function tuple(array[int] of int, array[int] of float): main(
	array[int] of int: x,
	array[int] of float: y
) = ([gini_lb([(x[i*2+1], x[i*2+2]) | i in 0..<(length(x) div 2)], 10000),], []);

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
