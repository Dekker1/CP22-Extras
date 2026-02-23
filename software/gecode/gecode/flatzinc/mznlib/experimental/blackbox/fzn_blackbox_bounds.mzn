predicate fzn_blackbox_bounds(
	array[int] of var int: int_input,
	array[int] of var float: float_input,
	array[int] of int: flat_reason,
) = gecode_blackbox_bounds(int_input, float_input, );

predicate gecode_blackbox_bounds(
	array[int] of var int: int_input,
	array[int] of var float: float_input,
);
