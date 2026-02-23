include "sliding_sum.mzn";

predicate fzn_sliding_among_int(int: low, int: up, int: l, array [int] of var int: x, set of int: S) =
    sliding_sum(low,up,l,[xi in S | xi in x]);
