include "experimental/blackbox/fzn_blackbox.mzn";
include "experimental/blackbox/fzn_blackbox_bounds.mzn";

annotation blackbox_exec(string: command);
annotation blackbox_dll(string: library_name);

/** @group blackbox.blackbox
  Instruct the solver to run a blackbox function when all decision variable in \a int_input and \a float_input are fixed, binding the output of the function to \a int_output and \a float_output.

  The blackbox function to use must be defined using the annotation `blackbox_exec` or `blackbox_dll`.
*/
predicate blackbox(
	array[int] of var int: int_input,
	array[int] of var float: float_input,
	array[int] of var int: int_output,
	array[int] of var float: float_output,
) = fzn_blackbox(
	int_input,
	float_input,
	int_output,
	float_output,
);

enum BBBound = {BB_LB, BB_UB};

/** @group blackbox.blackbox_bounds
  Instruct the solver to run a blackbox function if any bounds of the decision variable in \a int_input or \a float_input change, to compute new bounds for the decision variables.
  The blackbox function is provided with a lower bound and upper bound for each decision variable, and must return a lower bound and upper bound for each decision variable in the same order.
	\a reason provides a blueprint reason for any bounds change.

  The blackbox function to use must be defined using the annotation `blackbox_exec` or `blackbox_dll`.
*/
predicate blackbox_bounds(
	array[int] of var int: int_input,
	array[int] of var float: float_input,
	array[int] of array[int] of tuple(int, BBBound): reason,
)  = let {
	int: num_var = length(int_input) + length(float_input);
	constraint assert(length(reason) == 2*num_var, "blackbox_bound: expected \(num_var*2) reason descriptions for the bounds of \(num_var) decision variables, but \(length(reason)) reason blueprints provided");
	array[int] of int: flat_reason = [
		if j < 0 then
			length(x)
		elseif j mod 2 == 0 then
			x[k].1
		else
			enum2int(x[k].2)
		endif
	| i in index_set(reason),
		x = array1d(reason[i]),
		j in -1..length(x)*2-1,
		k = j div 2 + 1
	];
} in fzn_blackbox_bounds(
	int_input,
	float_input,
	flat_reason,
);
