include "experimental/blackbox.mzn";

predicate spread_BB(var int: v, array [int] of var int: X, var int: S, int: scale) =
	let {
		constraint assert(scale = 1, "scale has been fixed in the blackbox library");
		tuple(int, BBBound): rdummy = (1, BB_LB);
		array[int] of array[int] of tuple(int, BBBound): reason =
			[[rdummy] | i in 1..length(X)+1, j in 1..2]
			++ [
				[ (i, b) | i in index_set(X), b in {BB_LB, BB_UB} ],
				[rdummy],
			];
	} in blackbox_bounds(
		X ++ [S, v],
		[],
		reason,
	) ::blackbox_dll("/home/dekker1/bench_bb/software/install/lib/libbb_spread.so");

predicate gini_BB(var int: v, array [int] of var int: X, int: scale) =
	let {
		constraint assert(scale = 10000, "scale has been fixed in the blackbox library");
		tuple(int, BBBound): rdummy = (1, BB_LB);
		array[int] of array[int] of tuple(int, BBBound): reason =
			[[rdummy] | i in 1..length(X), j in 1..2]
			++ [
				[ (i, b) | i in index_set(X), b in {BB_LB, BB_UB} ],
				[rdummy],
			];
	} in blackbox_bounds(
		X ++ [v],
		[],
		reason,
	) ::blackbox_dll("/home/dekker1/bench_bb/software/install/lib/libbb_gini.so");
