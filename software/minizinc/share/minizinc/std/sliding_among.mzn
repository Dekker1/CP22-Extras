include "fzn_sliding_among_int.mzn";
include "fzn_sliding_among_bool.mzn";

/**  @group globals.counting
 Every subsequence of \a x of length \a seq has at least \a low and at most \a up occurrences
 of the values in \a v
*/
predicate sliding_among(int: low, int: up, int: seq, array [$X] of var $$E: x, set of $$E: v) =
  fzn_sliding_among_int(low, up, seq, enum2int(array1d(x)), enum2int(v));

/**  @group globals.counting
 Every subsequence of \a x of length \a seq has at least \a low and at most \a up occurrences
 of the value \a b
*/
predicate sliding_among(int: low, int: up, int: seq, array [$X] of var bool: x, bool: b) =
  fzn_sliding_among_bool(low, up, seq, array1d(x), b);
