include "../blackbox_impl.mzn";

% === Meta Parameters ============================================= %

int: mode; %% 0 (minnimal), -2 (decomp), 9 (lb), 10 (lb no daemon)

enum CASES = { MinDis % Minimise dispersion (bound on efficiency)
             , MaxEff % Maximise efficiency (bound on dispersion)
             , Mkspan % Minimise makespan for each agent (no
                      % constraints across agents). Used to find
                      % limited horizon size
             , MkspanPeter % Same as above but include constraints
                           % across agents
             , FndBnd % Maximise efficiency (no bound on dispersion).
                      % Used to find bound to use for MinDis/MaxEff
             };

CASES: Case;

% === Parameters ================================================== %

int: n_jobs;
int: n_machs;
int: n_agents;

set of int: A = 1..n_agents;
set of int: J = 1..n_jobs;
set of int: M = 1..n_machs;
set of int: T = M;

opt int: end;
opt int: bnd;

array [J,T] of int: dur;
array [J,T] of M: mac;
array [J] of A: alloc;

% === Variables =================================================== %

array [J,T] of var opt 0..infinity: start;
array [J] of var opt 0..infinity: done;

% `var opt' optimisation
constraint forall (j in J, t in T)
( if absent(start[j,t])
    then deopt(start[j,t]) = end+1
    else deopt(start[j,t]) <= end endif );

% === Meta Constraints ============================================ %

constraint if Case in {Mkspan, MkspanPeter} then
             forall (j in J)
              ( occurs(done[j]) )
           endif;

% === Scheduling Constraints ====================================== %
include "cumulative.mzn";

% Tasks are processed in the correct order
constraint forall (j in J, t in T diff {max(T)})
 ( deopt(start[j,t]) + occurs(start[j,t])*dur[j,t] <= deopt(start[j,enum_next(T, t)]) );

constraint forall (j in J, t in T)
 ( absent(start[j,t]) <-> deopt(start[j,t]) = end+1 ); % symmetry-breaking constraint

% Tasks do not overlap on a machine
constraint if Case = Mkspan then forall (m in M, a in A)
 ( cumulative( [ deopt(start[j,t])           | j in J, t in T where mac[j,t]=m /\ alloc[j] = a ],
               [ occurs(start[j,t])*dur[j,t] | j in J, t in T where mac[j,t]=m /\ alloc[j] = a ],
               [ 1                           | j in J, t in T where mac[j,t]=m /\ alloc[j] = a ],
                 1 )
 ) else forall (m in M)
 ( cumulative( [ deopt(start[j,t])           | j in J, t in T where mac[j,t]=m ],
               [ occurs(start[j,t])*dur[j,t] | j in J, t in T where mac[j,t]=m ],
               [ 1                           | j in J, t in T where mac[j,t]=m ],
                 1 )
 ) endif;

% start-done channelling
constraint forall (j in J)
 ( deopt(start[j,max(T)]) + dur[j,max(T)] = deopt(done[j]) );
constraint forall (j in J)
 ( done[j] <= end );
constraint forall (j in J, t in T)
 ( occurs(done[j]) <-> occurs(start[j,t]) );

% === Utilities =================================================== %

array [A] of var 0..infinity: util;
var 0..infinity: UTIL;
var 0..infinity: disp;

constraint UTIL = sum(util);

constraint forall (a in A)
 ( util[a] = sum (j in J, t in T where alloc[j] = a)
    ( occurs(done[j])*dur[j,t] ) );

constraint if Case in {MinDis, MaxEff} then
             spread(disp, util, UTIL, 1)
           else
             disp = 1
           endif;

% Bound on efficiency
constraint (Case = MinDis) ->
 ( UTIL >= bnd );

% Bound on dispersion
constraint (Case = MaxEff) ->
 ( disp <= bnd*1 );

% === Objective =================================================== %

var int: obj;

constraint if Case = MinDis then
             obj = disp % TODO
           elseif Case in {MaxEff, FndBnd} then
             obj = -UTIL % TODO
           else %if Case = Mkspan/MakespanPeter then
             forall (j in J)
              ( done[j] <= obj )
           endif;

ann: searchann = if Case in {Mkspan, MkspanPeter, FndBnd, MaxEff} then
                   seq_search([int_search([start[i,j] | i in J, j in T],
                                          smallest, indomain_min, complete ),
                               int_search([obj], input_order, indomain_min, complete)
                              ])
                 else % MinDis... Something smarter here?
                   seq_search([int_search([start[i,j] | i in J, j in T],
                                          smallest, indomain_min, complete ),
                               int_search([obj], input_order, indomain_min, complete)
                              ])
                 endif;


solve :: searchann minimize obj;

% === Predicates ================================================== %
%%% SPREAD
%% Modes: -2 = Decomposition as in paper.
%%         9 = Variance Lb Propagator as in paper.
%%         0 = Propagate variance when X fixed.

predicate spread(var int: v, array [int] of var int: X, var int: S, int: scale) =
	if mode = 666 then
		spread_BB(v, X, S, scale)
  elseif mode = -1 then
    spread_decomp(v, X, S, scale)
  elseif mode = -2 then
    spread_decomp2(X, S, v, scale)
  elseif mode = -3 then
    spread_decomp2(X, S, v, scale) /\ variance_int(v, X, S, scale, mode)
  else
    variance_int(v, X, S, scale, mode)
  endif;

predicate spread_decomp(var int: v, array [int] of var int: X, var int: S, int: scale) =
  let {int: N = card(index_set(X));
       array [int] of var 0..infinity: sq = [ abs(N*X[i] - S) * abs(N*X[i] - S) | i in index_set(X) ]}
  in v = (sum(sq)*scale) div (N*N*N);

predicate spread_decomp2(array[int] of var int:X, var int: M, var int: v, int:s) =
  let { int: n=length(X);
        array[int] of var 0..infinity: sq = [X[i]*X[i] | i in index_set(X)];
        %array[int] of var 0..infinity: sq2 = [ (n*X[i] - S) * (n*X[i] - S) | i in index_set(X) ];
        var 0..n*ub(sum(sq)): v_ = n*sum(sq);
        var lb(M)..infinity: Msq = M*M;
        var 0..ub(v_): v__ = v_ - Msq;
        var 0..infinity: v___ = v__*s;
        %constraint n*v_ = sum(sq2)*s;
        }
  in v = v___ div (n*n);

predicate variance_int(var int: y, array [int] of var int: x, var int: sum, int: s, int: mode);
