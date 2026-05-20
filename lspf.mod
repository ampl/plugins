function logistic;
function signpow;
set I = 1..3;
var x{i in I} := i in [-5,5];
minimize foo: logistic(x[1]) + signpow(x[2], x[3]);
display foo, x, x.rc;
write gtmp;
