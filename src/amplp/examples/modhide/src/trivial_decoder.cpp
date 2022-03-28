/*	Copyright (c) 2010 AMPL Optimization, Inc.	*/
/*	  All Rights Reserved 	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AMPL Optimization, Inc.	*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/* Sample crypto routine */

/** Adapted from the code available at
 * http://www.ampl.com/NEW/modhide.zip
 */

#include "amplp.hpp"

using namespace amplp;

 static char decoder_name[] = "trivial_decoder";

 static int
trivial_decoder(cryptblock *cb, char *s, char *b)
{
	char *se;
	size_t k, key;

	k = cb->bin;
	key = cb->state;
	if (k > cb->bout)
		k = cb->bout;
	se = s + k;
	while(s < se)
		*b++ = (char)(*s++ - key++);
	cb->state = key;
	cb->bin -= k;
	cb->bout = k;
	return 0;
	}

 static double
decrypt(arglist *al)
{
	AmplExports *ae;
	cryptblock *cb;

	ae = al->AE;
	cb = (*ae->Crypto)(decoder_name, 4);
	cb->state = (size_t)al->ra[0];
	cb->decoder = trivial_decoder;
	return 0.;
	}

 void
funcadd(AmplExports *ae)
{
	ae->Addfunc(decoder_name, (rfunc) decrypt, 0, 1, 0, ae);
	}
