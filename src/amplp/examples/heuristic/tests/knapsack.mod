param n >= 0;
param W >= 0;

param w{1..n} >= 0;
param v{1..n} >= 0;

var x{1..n} binary;

maximize profit: sum{i in 1..n} v[i]*x[i];
s.t. capacity_limit: sum{i in 1..n} w[i]*x[i] <= W;