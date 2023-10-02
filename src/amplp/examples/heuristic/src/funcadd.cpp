/****************************************************************
Copyright (C) 1997, 2000 Lucent Technologies
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of Lucent or any of its entities
not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
****************************************************************/

/** Adapted from
 * http://www.netlib.org/ampl/solvers/funclink/
 * /


/* sample funcadd */

#include <cmath>
#include "amplp.hpp"
#include "knap.hpp"


using namespace amplp;

KnapHeuristic myknap;


void
heuristic_show(arglist *al){

	myknap.show();
}


void
heuristic_solve(arglist *al){

	myknap.solve();
}


void
heuristic_set_capacity(arglist *al){

	int n = al->n;
	int nr = al->nr;

	if (nr > 0){
		myknap.set_capacity(al->ra[0]);
	}


};

void
heuristic_set_nitems(arglist *al){

	int n = al->n;
	int nr = al->nr;

	if (nr > 0){
		myknap.set_nitems(al->ra[0]);
	}
}


void
heuristic_set_values(arglist *al){

	int n = al->n;

	for (int i=0; i<n; i++){
		int val = al->ra[i];
		myknap.set_value(val, i);
	}
};


void
heuristic_set_weights(arglist *al){

	int n = al->n;

	for (int i=0; i<n; i++){
		int val = al->ra[i];
		myknap.set_weight(val, i);
	}
};

double
heuristic_get_obj(arglist *al){

	return myknap.get_obj();
};

double
heuristic_get_sol_val(arglist *al){

	return myknap.get_sol(al->ra[0]-1);
};


void
heuristic_get_sol(arglist *al){

	int n = al->n;

	for (int i=0; i<n; i++){
		al->ra[i] = myknap.get_sol(i);
		//~ std::cout << al->ra[i] << std::endl;
	}
};




void
funcadd(AmplExports *ae){
	/* Insert calls on addfunc here... */

/* Arg 3, called type, must satisfy 0 <= type <= 6:
 * type&1 == 0:	0,2,4,6	==> force all arguments to be numeric.
 * type&1 == 1:	1,3,5	==> pass both symbolic and numeric arguments.
 * type&6 == 0:	0,1	==> the function is real valued.
 * type&6 == 2:	2,3	==> the function is char * valued; static storage
			    suffices: AMPL copies the return value.
 * type&6 == 4:	4,5	==> the function is random (real valued).
 * type&6 == 6: 6	==> random, real valued, pass nargs real args,
 *				0 <= nargs <= 2.
 *
 * Arg 4, called nargs, is interpretted as follows:
 *	>=  0 ==> the function has exactly nargs arguments
 *	<= -1 ==> the function has >= -(nargs+1) arguments.
 *
 * Arg 5, called funcinfo, is copied without change to the arglist
 *	structure passed to the function; funcinfo is for the
 *	function to use or ignore as it sees fit.
 */

	/* Solvers quietly ignore kth, sginv, and rncall, since */
	/* kth and sginv are symbolic (i.e., char* valued) and  */
	/* rncall is specified as random.  Thus kth, sginv, and */
	/* rncall may not appear nonlinearly in declarations in */
	/* an AMPL model. */

	ae->Addfunc("heuristic_show", (ufunc*)heuristic_show, 0, 0, 0, ae);
	ae->Addfunc("heuristic_solve", (ufunc*)heuristic_solve, 0, 0, 0, ae);

	ae->Addfunc("heuristic_set_capacity", (ufunc*)heuristic_set_capacity, 0, 1, 0, ae);
	ae->Addfunc("heuristic_set_nitems", (ufunc*)heuristic_set_nitems, 0, 1, 0, ae);

	ae->Addfunc("heuristic_set_values", (rfunc)heuristic_set_values, 0, -1, 0, ae);
	ae->Addfunc("heuristic_set_weights", (rfunc)heuristic_set_weights, 0, -1, 0, ae);

	ae->Addfunc("heuristic_get_obj", (rfunc)heuristic_get_obj, 0, 0, 0, ae);
	ae->Addfunc("heuristic_get_sol_val", (rfunc)heuristic_get_sol_val, 0, 1, 0, ae);
	ae->Addfunc("heuristic_get_sol", (rfunc)heuristic_get_sol, FUNCADD_double_VALUED|FUNCADD_OUTPUT_ARGS, -1, 0, ae);


	/* at_end() and at_reset() calls could appear here, too. */
	}
