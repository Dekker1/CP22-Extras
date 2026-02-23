include "sliding_sum.mzn";

predicate fzn_sliding_among_bool(int: low, int: up, int: l, array [int] of var bool: x, bool: b) =
    sliding_sum(low,up,l,[xi=b | xi in x]);
