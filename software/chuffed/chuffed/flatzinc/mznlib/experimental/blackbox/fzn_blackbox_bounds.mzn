predicate fzn_blackbox_bounds(
	array[int] of var int: int_input,
	array[int] of var float: float_input,
	array[int] of int: flat_reason,
) = chuffed_blackbox_bounds(int_input, float_input, flat_reason);

predicate chuffed_blackbox_bounds(
	array[int] of var int: int_input,
	array[int] of var float: float_input,
	array[int] of int: flat_reason,
);
