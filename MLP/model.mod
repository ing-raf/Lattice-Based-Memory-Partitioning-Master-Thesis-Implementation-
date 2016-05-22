set P; # set of processors
set B; # set of memory banks
set T; # set of translates of the current fundamental lattice
set D; # set of different dataset types of the current fundamental lattice

param l; # service latency of the memory banks, assumed to be equal for all
param delta{P, B}; # time waited by the processor p for the data coming from the bank b
param n{D}; # number of repetitions of the dataset type d
param mc{D, P, T}; # number of conflicting memory accesses from the processor p to the translate t in the dataset type t
param minLatency; # best cost function value of the previous scanned fundamental lattices
param nonFirstLattice; # inverted flag to enable the constraint on the previous lattices

var x{T,B} binary; # 1 if the translate t is mapped on the memory bank b; 0 otherwise
var delay{D, T} >= 0; # maximum delay incurred in the dataset type d on the dataset t
var latency{D} >= 0; # latency of the dataset type d
var latticeLatency >= 0; # cost function value for the current translate

minimize cost: # cost function
	latticeLatency;

subject to bankToTranslateAssignment{b in B}: # each memory bank must be mapped to at most one translate
	sum{t in T} x[t,b] <= 1;

subject to translateToBankAssignment{t in T}: # each translate must be mapped to exactly one memory bank
	sum{b in B} x[t,b] = 1;

# a constraint for each p to take the maximum over P
subject to dataSetTypeTranslateLatency{d in D, t in T, p in P : mc[d, p, t] > 0}: # expression of the maximum delay incurred in the dataset t
	delay[d,t] >= sum{b in B} x[t, b] * delta[p, b];

# a constraint for each t to take the maximum over T
subject to dataSetTypeLatency{d in D, t in T}: # expression of the delay incurred by each dataset type
	latency[d] >= sum{p in P} (l * mc[d, p, t]) + delay[d,t];
	# latency[d] >= sum{p in P, b in B} (l[b] * x[t, b] * mc[d, p, t]) + delay[d,t];

subject to totalLatencyComputation: # cost function expression, written as a constraint to enable the constraint with the current best fundamental lattice
	latticeLatency = sum{d in D} n[d]*latency[d];	

subject to maxLatencyConstraint: # the current fundamental lattice must be better than the current best
	nonFirstLattice * latticeLatency <= minLatency;

end;
