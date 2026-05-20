#include "math.h"	/* for log, exp */
#include "funcadd.h"	/* includes "stdio1.h" */

 static real
logistic(arglist *al) /* logistic(x) = 1 / (1 + exp(-x) */
{
	real *d, *h, e, rv, x, y;

	if (al->n != 1 || al->nr != 1) {
		al->Errmsg = "Expected one real arg";
		return 0;
		}
	x = al->ra[0];
	e = exp(-x);
	rv = 1. / (1. + e);
	if ((d = al->derivs)) {
		d[0] = (rv * e) * rv;
		if ((h = al->hes))
			h[0] = e*rv*(d[0]+d[0] - rv);
		}
	return rv;
	}

 static real
signpow(arglist *al) /* signpow(x,y) = sign(x)*|x|^y */
{
	real *d, *h, rv, s, t1, t2, u, x, y;

	if (al->n != 2 || al->nr != 2) {
		al->Errmsg = "Expected two real args";
		return 0;
		}
	s = 1.;
	if ((x = al->ra[0]) < 0.) {
		s = -1.;
		x = -x;
		} 
	y = al->ra[1];
	u = pow(x,y);
	rv = s*u;
	if ((d = al->derivs)) {
		d[0] = y*(t1 = u/x);
		d[1] = (t2 = log(x))*rv;
		if ((h = al->hes)) {
			h[0] = s*(y-1.)*(d[0]/x);
			h[1] = t1 * (1. + y*t2);
			h[2] = t2*d[1];
			}
		}
	return rv;
	}

 void
funcadd(AmplExports *ae)
{
	addfunc("logistic", (rfunc)logistic, 1, 1, 0);
	addfunc("signpow", (rfunc)signpow, 1, 2, 0);
	}
